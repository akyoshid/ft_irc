/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: reasuke <reasuke@student.42tokyo.jp>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/03 23:57:17 by akyoshid          #+#    #+#             */
/*   Updated: 2025/11/09 00:00:00 by reasuke          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include "server/CommandParser.hpp"
#include "server/ConnectionManager.hpp"
#include "server/EventLoop.hpp"
#include "utils.hpp"

extern volatile sig_atomic_t g_shutdown;

Server::Server(const std::string& portStr, const std::string& password)
    : password_(password), serverSocket_(INVALID_FD) {
  validateAndSetPort(portStr);
  validatePassword(password);
  setupServerSocket();
}

Server::~Server() {
  connManager_.disconnectAll();
  if (serverSocket_ != INVALID_FD) close(serverSocket_);
}

// ==========================================
// Main event loop
// ==========================================
void Server::run() {
  struct epoll_event events[MAX_EVENTS];

  while (!g_shutdown) {
    int nfds = eventLoop_.wait(events, MAX_EVENTS, 30000);
    if (nfds < 0) {
      if (errno == EINTR) {
        // Interrupted by signal
        continue;
      }
      throw std::runtime_error(
          createLog(LOG_LEVEL_ERROR, LOG_CATEGORY_SYSTEM,
                    createErrorMessage("epoll_wait", errno)));
    }

    for (int i = 0; i < nfds; ++i) {
      handleEvent(events[i]);
    }
  }
}

// ==========================================
// Event handling
// ==========================================
void Server::handleEvent(const struct epoll_event& event) {
  int fd = event.data.fd;
  uint32_t events = event.events;

  // Server socket: new connection
  if (fd == serverSocket_) {
    if (events & EPOLLIN) {
      acceptConnections();
    }
    return;
  }

  // Client socket: error handling
  if (events & (EPOLLERR | EPOLLHUP)) {
    handleClientError(fd);
    return;
  }

  // Client socket: data I/O
  Client* client = connManager_.getClient(fd);
  if (!client) {
    log(LOG_LEVEL_WARNING, LOG_CATEGORY_CONNECTION,
        "Event for non-existent client");
    return;
  }

  if (events & EPOLLIN) {
    handleClientRead(client);
  }

  // Re-check if client still exists after read (might have disconnected)
  if (events & EPOLLOUT) {
    client = connManager_.getClient(fd);
    if (client) {
      handleClientWrite(client);
    }
  }
}

void Server::acceptConnections() {
  // Edge-triggered: accept all pending connections
  while (true) {
    Client* newClient = connManager_.acceptConnection(serverSocket_);
    if (!newClient) break;  // No more connections (EAGAIN)

    // Check client limit to prevent resource exhaustion
    // Note: acceptConnection() already added the client to the map
    if (connManager_.getClients().size() > MAX_CLIENTS) {
      log(LOG_LEVEL_WARNING, LOG_CATEGORY_CONNECTION,
          "Maximum client limit reached, rejecting connection from " +
              newClient->getIp());
      connManager_.disconnect(newClient->getSocketFd());
      continue;
    }

    eventLoop_.addFd(newClient->getSocketFd(), EPOLLIN);

    log(LOG_LEVEL_INFO, LOG_CATEGORY_CONNECTION,
        "New connection: " + newClient->getIp());

    // Send initial message
    newClient->getWriteBuffer() +=
        ":ft_irc NOTICE * :Please authenticate with PASS command\r\n";
    eventLoop_.modifyFd(newClient->getSocketFd(), EPOLLIN | EPOLLOUT);
  }
}

void Server::handleClientError(int fd) {
  Client* client = connManager_.getClient(fd);
  if (client) {
    log(LOG_LEVEL_WARNING, LOG_CATEGORY_CONNECTION,
        "Connection closed unexpectedly: " + client->getIp());
    disconnectClient(fd);
  }
}

void Server::handleClientRead(Client* client) {
  std::vector<std::string> messages;

  // Receive data
  ReceiveResult result = connManager_.receiveData(client, messages);

  if (result == RECV_CLOSED || result == RECV_ERROR) {
    disconnectClient(client->getSocketFd());
    return;
  }

  // Process received messages
  for (size_t i = 0; i < messages.size(); ++i) {
    cmdParser_.processMessage(client, messages[i]);
  }
}

void Server::handleClientWrite(Client* client) {
  SendResult result = connManager_.sendData(client);

  if (result == SEND_ERROR) {
    disconnectClient(client->getSocketFd());
    return;
  }

  // All data sent: remove EPOLLOUT
  if (result == SEND_COMPLETE) {
    eventLoop_.modifyFd(client->getSocketFd(), EPOLLIN);
  }
}

void Server::disconnectClient(int fd) {
  eventLoop_.removeFd(fd);
  connManager_.disconnect(fd);
}

// ==========================================
// Setup and validation
// ==========================================
void Server::validateAndSetPort(const std::string& portStr) {
  if (portStr.length() != 4)
    throw std::runtime_error(
        "Invalid port: must be exactly 4 digits (6665-6669)");

  port_ = 0;
  for (size_t i = 0; i < 4; ++i) {
    if (!std::isdigit(static_cast<unsigned char>(portStr[i])))
      throw std::runtime_error("Invalid port: contains non-digit characters");
    port_ = port_ * 10 + (portStr[i] - '0');
  }

  if (port_ < 6665 || port_ > 6669) {
    throw std::runtime_error("Invalid port: allowed range is 6665-6669");
  }
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void Server::validatePassword(const std::string& password) {
  size_t len = password.length();
  if (len < 8)
    throw std::runtime_error("Invalid password: must be at least 8 characters");
  if (len > 64)
    throw std::runtime_error("Invalid password: must be at most 64 characters");

  for (size_t i = 0; i < len; ++i) {
    if (!std::isgraph(static_cast<unsigned char>(password[i])))
      throw std::runtime_error(
          "Invalid password: "
          "contains non-printable or space characters");
  }
}

void Server::setupServerSocket() {
  // Create TCP socket for IPv4
  serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
  if (serverSocket_ < 0) {
    int errsv = errno;
    throw std::runtime_error(createErrorMessage("socket", errsv));
  }

  // SO_REUSEADDR: Allow quick server restart
  int opt = 1;
  if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
      0) {
    int errsv = errno;
    throw std::runtime_error(createErrorMessage("setsockopt", errsv));
  }

  // Set non-blocking mode
  if (fcntl(serverSocket_, F_SETFL, O_NONBLOCK) < 0) {
    int errsv = errno;
    throw std::runtime_error(createErrorMessage("fcntl", errsv));
  }

  // Bind to address
  struct sockaddr_in address;
  std::memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port_);

  if (bind(serverSocket_, reinterpret_cast<struct sockaddr*>(&address),
           sizeof(address)) < 0) {
    int errsv = errno;
    throw std::runtime_error(createErrorMessage("bind", errsv));
  }

  // Listen for connections
  if (listen(serverSocket_, MAX_QUEUE) < 0) {
    int errsv = errno;
    throw std::runtime_error(createErrorMessage("listen", errsv));
  }

  // Create epoll instance and register server socket
  eventLoop_.create();
  eventLoop_.addFd(serverSocket_, EPOLLIN);

  log(LOG_LEVEL_INFO, LOG_CATEGORY_SYSTEM,
      "Server started listening on port " + int_to_string(port_));
}
