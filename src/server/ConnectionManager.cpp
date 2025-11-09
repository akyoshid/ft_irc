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

#include "Client.hpp"
#include "utils.hpp"

ConnectionManager::ConnectionManager() {}

ConnectionManager::~ConnectionManager() { disconnectAll(); }

Client* ConnectionManager::acceptConnection(int serverFd) {
  struct sockaddr_in clientAddr;
  socklen_t clientAddrLen = sizeof(clientAddr);

  // Accept new connection
  // This creates a new socket file descriptor for the client
  int clientFd =
      accept(serverFd, reinterpret_cast<struct sockaddr*>(&clientAddr),
             &clientAddrLen);
  if (clientFd < 0) {
    // No more connections waiting (normal for edge-triggered mode)
    if (errno == EAGAIN || errno == EWOULDBLOCK) return NULL;
    // Unexpected error
    throw std::runtime_error(createLog(LOG_LEVEL_ERROR, LOG_CATEGORY_SYSTEM,
                                       createErrorMessage("accept", errno)));
  }

  // Set the new client socket to non-blocking mode
  if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0) {
    int errsv = errno;
    close(clientFd);
    throw std::runtime_error(createLog(LOG_LEVEL_ERROR, LOG_CATEGORY_SYSTEM,
                                       createErrorMessage("fcntl", errsv)));
  }

  // Get client IP address
  char clientIp[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, INET_ADDRSTRLEN);

  // Create and store new client
  // Use try-catch for exception safety (issue #24)
  Client* newClient = NULL;
  try {
    newClient = new Client(clientFd, std::string(clientIp));
    clients_[clientFd] = newClient;
  } catch (...) {
    delete newClient;
    close(clientFd);
    throw;
  }

  return newClient;
}

void ConnectionManager::disconnect(int clientFd) {
  std::map<int, Client*>::iterator it = clients_.find(clientFd);
  if (it == clients_.end()) {
    log(LOG_LEVEL_WARNING, LOG_CATEGORY_CONNECTION,
        "Attempted to disconnect a non-existent client");
    return;
  }

  std::string ip = it->second->ip_;
  close(clientFd);
  delete it->second;
  clients_.erase(it);

  log(LOG_LEVEL_INFO, LOG_CATEGORY_CONNECTION,
      "Disconnected successfully: " + ip);
}

void ConnectionManager::disconnectAll() {
  for (std::map<int, Client*>::iterator it = clients_.begin();
       it != clients_.end(); ++it) {
    close(it->first);
    delete it->second;
  }
  clients_.clear();
}

ReceiveResult ConnectionManager::receiveData(
    Client* client, std::vector<std::string>& messages) {
  char buffer[BUFFER_SIZE];

  // Read all available data (edge-triggered mode)
  while (true) {
    ssize_t bytesRead = recv(client->socketFd_, buffer, BUFFER_SIZE - 1, 0);

    if (bytesRead > 0) {
      client->readBuffer_.append(buffer, bytesRead);

      // Remove all Ctrl-D (EOT, '\x04') characters from the buffer
      client->readBuffer_.erase(std::remove(client->readBuffer_.begin(),
                                            client->readBuffer_.end(), '\x04'),
                                client->readBuffer_.end());

      // Prevent DoS attacks by limiting buffer size
      if (client->readBuffer_.size() > MAX_BUFFER_SIZE) {
        log(LOG_LEVEL_ERROR, LOG_CATEGORY_CONNECTION,
            "Read buffer is too large: " + client->ip_);
        return RECV_ERROR;
      }

      // Extract complete messages (ending with \r\n)
      size_t pos = client->readBuffer_.find("\r\n");
      while (pos != std::string::npos) {
        std::string message = client->readBuffer_.substr(0, pos);
        client->readBuffer_.erase(0, pos + 2);  // Also delete "\r\n"
        if (!message.empty()) {
          messages.push_back(message);
        }
        pos = client->readBuffer_.find("\r\n");
      }
    } else if (bytesRead == 0) {
      // The client closed the connection
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

SendResult ConnectionManager::sendData(Client* client) {
  if (client->writeBuffer_.empty()) {
    log(LOG_LEVEL_WARNING, LOG_CATEGORY_CONNECTION,
        "Attempted to send, but write buffer is empty");
    return SEND_COMPLETE;
  }

  while (!client->writeBuffer_.empty()) {
    ssize_t bytesSent = send(client->socketFd_, client->writeBuffer_.c_str(),
                             client->writeBuffer_.size(), MSG_NOSIGNAL);
    if (bytesSent > 0) {
      // Remove the successfully sent data from the buffer
      client->writeBuffer_.erase(0, bytesSent);
    } else if (bytesSent < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // Send buffer is full (retry later)
        return SEND_SUCCESS;
      }
      // send() failed
      log(LOG_LEVEL_ERROR, LOG_CATEGORY_SYSTEM,
          createErrorMessage("send", errno));
      return SEND_ERROR;
    }
  }

  // All data sent
  return SEND_COMPLETE;
}

Client* ConnectionManager::getClient(int fd) {
  std::map<int, Client*>::iterator it = clients_.find(fd);
  if (it == clients_.end()) return NULL;
  return it->second;
}

const std::map<int, Client*>& ConnectionManager::getClients() const {
  return clients_;
}
