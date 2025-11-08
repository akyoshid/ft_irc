/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: reasuke <reasuke@student.42tokyo.jp>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/03 23:57:17 by akyoshid          #+#    #+#             */
/*   Updated: 2025/11/08 18:02:31 by reasuke          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

#include "utils.hpp"

extern volatile sig_atomic_t g_shutdown;

Server::Server(const std::string& portStr, const std::string& password)
    : serverSocket_(INVALID_FD), epollFd_(INVALID_FD) {
  setPort(portStr);
  setPassword(password);
  setServerSocket();
}

Server::~Server() {
  for (std::map<int, Client*>::iterator it = clients_.begin();
       it != clients_.end(); ++it) {
    close(it->first);
    delete it->second;
  }
  if (serverSocket_ != INVALID_FD) close(serverSocket_);
  if (epollFd_ != INVALID_FD) close(epollFd_);
}

// Validate and convert a port string to an int.
// Only accepts exactly 4 digits
//  and the range 6665-6669 (IRC standard ports).
// No sign, no spaces.
// Throws std::runtime_error on invalid input.
void Server::setPort(const std::string& portStr) {
  if (portStr.length() != 4)
    throw(std::runtime_error(
        "Invalid port: must be exactly 4 digits (6665-6669)"));
  port_ = 0;
  for (size_t i = 0; i < 4; ++i) {
    if (!std::isdigit(static_cast<unsigned char>(portStr[i])))
      throw(std::runtime_error("Invalid port: contains non-digit characters"));
    port_ = port_ * 10 + (portStr[i] - '0');
  }
  if (port_ < 6665 || port_ > 6669) {
    throw(std::runtime_error("Invalid port: allowed range is 6665-6669"));
  }
}

// Validate password string and return it by reference.
// Requirements: printable (no spaces), length between 8 and 64.
// Throws std::runtime_error on invalid input.
void Server::setPassword(const std::string& password) {
  size_t len = password.length();
  if (len < 8)
    throw(
        std::runtime_error("Invalid password: must be at least 8 characters"));
  if (len > 64)
    throw(
        std::runtime_error("Invalid password: must be at most 64 characters"));
  for (size_t i = 0; i < len; ++i) {
    if (!std::isgraph(static_cast<unsigned char>(password[i])))
      throw(
          std::runtime_error("Invalid password: "
                             "contains non-printable or space characters"));
  }
  password_ = password;
}

// Create a TCP socket for IPv4
void Server::setServerSocket() {
  // Prototype: int socket(int domain, int type, int protocol);
  // AF_INET: IPv4 address family
  // SOCK_STREAM: TCP socket type (reliable, connection-oriented)
  // 0: Let system choose appropriate protocol (TCP in this case)
  serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
  if (serverSocket_ < 0) {
    int errsv = errno;
    throw(std::runtime_error(createErrorMessage("socket", errsv)));
  }
  // This option allows the socket to bind to an address that is
  // in TIME_WAIT state. Very important for server restart scenarios.
  // Without this, you'd get "Address already in use" error when
  // restarting your IRC server quickly after shutdown.
  int opt = 1;
  if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
      0) {
    int errsv = errno;
    throw(std::runtime_error(createErrorMessage("setsockopt", errsv)));
  }
  // Set socket to non-blocking mode
  if (fcntl(serverSocket_, F_SETFL, O_NONBLOCK) < 0) {
    int errsv = errno;
    throw(std::runtime_error(createErrorMessage("fcntl", errsv)));
  }
  // Bind socket to the specified address
  // "Address" is the combination of an IP address and a port number
  // AF_INET: IPv4 address family
  // INADDR_ANY: Accept connections from any IP address
  // htons(): Convert port number to network byte order
  struct sockaddr_in address;
  std::memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port_);
  if (bind(serverSocket_, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) < 0) {
    int errsv = errno;
    throw(std::runtime_error(createErrorMessage("bind", errsv)));
  }
  // Start listening for incoming connections
  // MAX_QUEUE of 8 means up to 8 pending connections can queue
  if (listen(serverSocket_, MAX_QUEUE) < 0) {
    int errsv = errno;
    throw(std::runtime_error(createErrorMessage("listen", errsv)));
  }
  // Create an epoll instance to start monitoring events
  // The argument 0 means "no special flags"
  epollFd_ = epoll_create1(0);
  if (epollFd_ < 0) {
    int errsv = errno;
    throw(std::runtime_error(createErrorMessage("epoll_create1", errsv)));
  }
  // Prepare the event to watch
  // EPOLLIN: watch for readability on the server socket,
  //  which indicates a new incoming connection
  // EPOLLET: edge-triggered mode - event is triggered only when
  //  the state changes (e.g., when new connection arrives).
  //  This requires draining all pending connections in a loop.
  // data: store the fd so epoll_wait can return it later
  struct epoll_event ev;
  ev.events = EPOLLIN | EPOLLET;
  ev.data.fd = serverSocket_;
  // Register the server socket fd with the epoll instance
  // Note: epoll_ctl still requires the target fd as a separate argument.
  // The third parameter to epoll_ctl tells the kernel which file descriptor
  // to add/modify/delete in the epoll instance (it is the operation key).
  // ev is just metadata (events + user-data) that the kernel associates
  // with that fd and will return via epoll_wait.
  if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, serverSocket_, &ev) < 0) {
    int errsv = errno;
    throw(std::runtime_error(createErrorMessage("epoll_ctl", errsv)));
  }
  log(LOG_LEVEL_INFO, LOG_CATEGORY_SYSTEM,
      "Server started listening on port " + int_to_string(port_));
}

// ==========================================
// epoll
// ==========================================
void Server::addToEpoll(int fd, uint32_t events) const {
  struct epoll_event ev;
  ev.events = events | EPOLLET;
  ev.data.fd = fd;
  if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
    throw std::runtime_error(createLog(LOG_LEVEL_ERROR, LOG_CATEGORY_SYSTEM,
                                       createErrorMessage("epoll_ctl", errno)));
  }
}

void Server::modifyEpollEvents(int fd, uint32_t events) const {
  struct epoll_event ev;
  ev.events = events | EPOLLET;
  ev.data.fd = fd;
  if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev) < 0) {
    throw(
        std::runtime_error(createLog(LOG_LEVEL_ERROR, LOG_CATEGORY_SYSTEM,
                                     createErrorMessage("epoll_ctl", errno))));
  }
}

void Server::removeFromEpoll(int fd) const {
  if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, NULL) < 0) {
    throw(
        std::runtime_error(createLog(LOG_LEVEL_ERROR, LOG_CATEGORY_SYSTEM,
                                     createErrorMessage("epoll_ctl", errno))));
  }
}

// ==========================================
// Client connection
// ==========================================
void Server::acceptNewConnection() {
  struct sockaddr_in clientAddr;
  socklen_t clientAddrLen = sizeof(clientAddr);
  // Accept all pending connections because it's edge-triggered mode
  while (1) {
    // Accept new connection
    // This creates a new socket file descriptor for the client
    int clientFd =
        accept(serverSocket_, reinterpret_cast<struct sockaddr*>(&clientAddr), &clientAddrLen);
    if (clientFd < 0) {
      // No more waiting for connections
      if (errno == EAGAIN || errno == EWOULDBLOCK) return;
      // unexpected error
      throw(std::runtime_error(createLog(LOG_LEVEL_ERROR, LOG_CATEGORY_SYSTEM,
                                         createErrorMessage("accept", errno))));
    }
    // Set the new client socket to non-blocking mode
    if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0) {
      close(clientFd);
      throw(std::runtime_error(createLog(LOG_LEVEL_ERROR, LOG_CATEGORY_SYSTEM,
                                         createErrorMessage("fcntl", errno))));
    }
    // Save the new client to the client list
    char clientIp[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, INET_ADDRSTRLEN);
    Client* newClient = new Client(clientFd, std::string(clientIp));
    clients_[clientFd] = newClient;
    // Register the new client socket fd with the epoll instance
    addToEpoll(clientFd, EPOLLIN);
    // Print client information
    log(LOG_LEVEL_INFO, LOG_CATEGORY_CONNECTION,
        "New connection: " + newClient->ip_);
    // Add first message to send buffer
    newClient->writeBuffer_ +=
        ":ft_irc NOTICE * :Please authenticate with PASS command\r\n";
    // Update epoll so it sends when sending becomes possible
    modifyEpollEvents(clientFd, EPOLLIN | EPOLLOUT);
  }
}

void Server::disconnectClient(int clientFd) {
  std::map<int, Client*>::iterator it = clients_.find(clientFd);
  if (it == clients_.end()) {
    log(LOG_LEVEL_WARNING, LOG_CATEGORY_CONNECTION,
        "Attempted to disconnect with a non-existent client");
    return;
  }
  removeFromEpoll(clientFd);
  close(clientFd);
  std::string ip = it->second->ip_;
  delete it->second;
  clients_.erase(it);
  log(LOG_LEVEL_INFO, LOG_CATEGORY_CONNECTION,
      "Disconnected successfully: " + ip);
}

// ==========================================
// Communication with the client
// ==========================================
void Server::recvFromClient(Client* client) {
  char buffer[BUFFER_SIZE];
  // Read all available data because it's edge-triggered mode
  while (1) {
    ssize_t bytesRead = recv(client->socketFd_, buffer, BUFFER_SIZE - 1, 0);
    if (bytesRead > 0) {
      client->readBuffer_.append(buffer, bytesRead);
      // Remove all Ctrl-D (EOT, '\x04') characters from the buffer.
      client->readBuffer_.erase(std::remove(client->readBuffer_.begin(),
                                            client->readBuffer_.end(), '\x04'),
                                client->readBuffer_.end());
      // Prevent DoS attacks
      if (client->readBuffer_.size() > MAX_BUFFER_SIZE) {
        log(LOG_LEVEL_ERROR, LOG_CATEGORY_CONNECTION,
            "Read buffer is too large, try to disconnect: " + client->ip_);
        disconnectClient(client->socketFd_);
        return;
      }
      size_t pos = client->readBuffer_.find("\r\n");
      while (pos != std::string::npos) {
        std::string message = client->readBuffer_.substr(0, pos);
        client->readBuffer_.erase(0, pos + 2);  // Also delete "\r\n"
        if (!message.empty()) processMessage(client, message);
        pos = client->readBuffer_.find("\r\n");
      }
    } else if (bytesRead == 0) {
      // The client closed the connection
      disconnectClient(client->socketFd_);
      return;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      // No more data available (normal for non-blocking socket)
      return;
    }
    // recv() failed
    log(LOG_LEVEL_ERROR, LOG_CATEGORY_SYSTEM,
        createErrorMessage("recv", errno));
    disconnectClient(client->socketFd_);
    return;
  }
}

void Server::sendToClient(Client* client) {
  if (client->writeBuffer_.empty()) {
    log(LOG_LEVEL_WARNING, LOG_CATEGORY_CONNECTION,
        "Attempted to send a message, but the message did not exist");
    return;
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
        return;
      }
      // send() failed
      log(LOG_LEVEL_ERROR, LOG_CATEGORY_SYSTEM,
          createErrorMessage("send", errno));
      disconnectClient(client->socketFd_);
      return;
    }
  }
  // Remove the EPOLLOUT event after all send() are completed
  modifyEpollEvents(client->socketFd_, EPOLLIN);
}

// ==========================================
// run
// ==========================================
void Server::run() {
  while (!g_shutdown) {
    int nfds = epoll_wait(epollFd_, events_, MAX_EVENTS, 30000);
    if (nfds < 0) {
      if (errno == EINTR) {
        // In the case of interruption by a signal
        continue;
      }
      throw(std::runtime_error(
          createLog(LOG_LEVEL_ERROR, LOG_CATEGORY_SYSTEM,
                    createErrorMessage("epoll_wait", errno))));
    }
    for (int i = 0; i < nfds; ++i) {
      int currentFd = events_[i].data.fd;
      uint32_t currentEvents = events_[i].events;
      if (currentFd == serverSocket_) {
        if (currentEvents & EPOLLIN) acceptNewConnection();
      } else {
        if (currentEvents & (EPOLLERR | EPOLLHUP)) {
          // Handle unexpected disconnections
          log(LOG_LEVEL_WARNING, LOG_CATEGORY_CONNECTION,
              "Connection closed unexpectedly: " +
                  clients_.find(currentFd)->second->ip_);
          disconnectClient(currentFd);
          continue;
        }
        std::map<int, Client*>::iterator it = clients_.find(currentFd);
        if (it != clients_.end()) {
          Client* currentClient = it->second;
          if (currentEvents & EPOLLIN) recvFromClient(currentClient);
          if (currentEvents & EPOLLOUT) sendToClient(currentClient);
        } else {
          log(LOG_LEVEL_WARNING, LOG_CATEGORY_CONNECTION,
              "Attempted to communicate with a non-existent client");
        }
      }
    }
  }
}

// ==========================================
// Process a message
// ==========================================
// NOTE: Currently static as it doesn't access Server member variables.
// This may change in the future when implementing IRC command handling
// that requires access to server state (e.g., password_, clients_).
void Server::processMessage(Client* client, const std::string& message) {
  // tmp
  log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND, client->ip_ + ": " + message);
  // // After completing this function,
  // //  output to the log what command resulted from the parsing.
  // log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND,
  //     client->ip_ + ": " + <command> + <args...>);
}
