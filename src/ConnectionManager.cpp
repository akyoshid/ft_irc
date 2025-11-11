#include "ConnectionManager.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>

#include "User.hpp"
#include "utils.hpp"

ConnectionManager::ConnectionManager() {}

ConnectionManager::~ConnectionManager() {}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
User* ConnectionManager::acceptConnection(int serverFd) {
  struct sockaddr_in userAddr;
  socklen_t userAddrLen = sizeof(userAddr);

  // Accept new connection
  // This creates a new socket file descriptor for the user
  int userFd = accept(serverFd, reinterpret_cast<struct sockaddr*>(&userAddr),
                      &userAddrLen);
  if (userFd < 0) {
    // No more connections waiting (normal for edge-triggered mode)
    if (errno == EAGAIN || errno == EWOULDBLOCK) return NULL;
    // Unexpected error
    throw std::runtime_error(createLog(LOG_LEVEL_ERROR, LOG_CATEGORY_SYSTEM,
                                       createErrorMessage("accept", errno)));
  }

  // Set the new user socket to non-blocking mode
  if (fcntl(userFd, F_SETFL, O_NONBLOCK) < 0) {
    int errsv = errno;
    close(userFd);
    throw std::runtime_error(createLog(LOG_LEVEL_ERROR, LOG_CATEGORY_SYSTEM,
                                       createErrorMessage("fcntl", errsv)));
  }

  // Get user IP address
  char userIp[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &userAddr.sin_addr, userIp, INET_ADDRSTRLEN);

  // Create new user
  // Use try-catch for exception safety (issue #24)
  User* newUser = NULL;
  try {
    newUser = new User(userFd, std::string(userIp));
  } catch (...) {
    delete newUser;  // NULL-safe in C++
    close(userFd);
    throw;
  }

  return newUser;
}

ReceiveResult ConnectionManager::receiveData(
    User* user, std::vector<std::string>& messages) {
  (void)this;  // Suppress unused warning
  char buffer[BUFFER_SIZE];

  // Read all available data (edge-triggered mode)
  while (true) {
    ssize_t bytesRead = recv(user->getSocketFd(), buffer, BUFFER_SIZE - 1, 0);

    if (bytesRead > 0) {
      user->getReadBuffer().append(buffer, bytesRead);

      // Remove all Ctrl-D (EOT, '\x04') characters from the buffer
      std::string& readBuf = user->getReadBuffer();
      readBuf.erase(std::remove(readBuf.begin(), readBuf.end(), '\x04'),
                    readBuf.end());

      // Prevent DoS attacks by limiting buffer size
      if (readBuf.size() > MAX_BUFFER_SIZE) {
        log(LOG_LEVEL_ERROR, LOG_CATEGORY_CONNECTION,
            "Read buffer is too large: " + user->getIp());
        return RECV_ERROR;
      }

      // Extract complete messages (ending with \r\n)
      size_t pos = readBuf.find("\r\n");
      while (pos != std::string::npos) {
        std::string message = readBuf.substr(0, pos);
        readBuf.erase(0, pos + 2);  // Also delete "\r\n"
        if (!message.empty()) {
          messages.push_back(message);
        }
        pos = readBuf.find("\r\n");
      }
    } else if (bytesRead == 0) {
      // The user closed the connection
      return RECV_CLOSED;
    } else {
      // bytesRead < 0
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // No more data available (normal for non-blocking socket)
        return RECV_SUCCESS;
      }
      // recv() failed
      log(LOG_LEVEL_ERROR, LOG_CATEGORY_SYSTEM,
          createErrorMessage("recv", errno));
      return RECV_ERROR;
    }
  }
}

SendResult ConnectionManager::sendData(User* user) {
  (void)this;  // Suppress unused warning
  std::string& writeBuf = user->getWriteBuffer();

  if (writeBuf.empty()) {
    log(LOG_LEVEL_WARNING, LOG_CATEGORY_CONNECTION,
        "Attempted to send, but write buffer is empty");
    return SEND_COMPLETE;
  }

  size_t totalSent = 0;

  while (!writeBuf.empty()) {
    ssize_t bytesSent = send(user->getSocketFd(), writeBuf.c_str(),
                             writeBuf.size(), MSG_NOSIGNAL);
    if (bytesSent > 0) {
      writeBuf.erase(0, bytesSent);
      totalSent += bytesSent;
    } else if (bytesSent < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // Explicitly log when the send buffer is full
        if (totalSent == 0) {
          log(LOG_LEVEL_DEBUG, LOG_CATEGORY_NETWORK,
              "Send buffer full for " + user->getIp() +
                  " (queued: " + int_to_string(writeBuf.size()) + " bytes)");
        } else {
          log(LOG_LEVEL_DEBUG, LOG_CATEGORY_NETWORK,
              "Partial send for " + user->getIp() +
                  " (sent: " + int_to_string(totalSent) +
                  ", remaining: " + int_to_string(writeBuf.size()) + " bytes)");
        }
        return SEND_SUCCESS;
      }
      log(LOG_LEVEL_ERROR, LOG_CATEGORY_SYSTEM,
          createErrorMessage("send", errno));
      return SEND_ERROR;
    }
  }

  // All data transmission complete
  log(LOG_LEVEL_DEBUG, LOG_CATEGORY_NETWORK,
      "Sent " + int_to_string(totalSent) + " bytes to " + user->getIp());
  return SEND_COMPLETE;
}
