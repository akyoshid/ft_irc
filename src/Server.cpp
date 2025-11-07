/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akyoshid <akyoshid@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/03 23:57:17 by akyoshid          #+#    #+#             */
/*   Updated: 2025/11/07 15:56:05 by akyoshid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <string>
#include <cctype>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <cerrno>
#include "../include/Server.hpp"
#include "../include/utils.hpp"

Server::Server(const std::string& portStr, const std::string& password)
    : serverSocket_(INVALID_FD) {
    setPort(portStr);
    setPassword(password);
    setServerSocket();
    std::cout << "port: " << port_ << "\n";
    std::cout << "password: " << password_ << "\n";
}

Server::~Server() {
    if (serverSocket_ != INVALID_FD)
        close(serverSocket_);
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
            throw(std::runtime_error(
                "Invalid port: contains non-digit characters"));
        port_ = port_ * 10 + (portStr[i] - '0');
    }
    if (port_ < 6665 || port_ > 6669) {
        throw(std::runtime_error(
            "Invalid port: allowed range is 6665-6669"));
    }
}

// Validate password string and return it by reference.
// Requirements: printable (no spaces), length between 8 and 64.
// Throws std::runtime_error on invalid input.
void Server::setPassword(const std::string& password) {
    size_t len = password.length();
    if (len < 8)
        throw(std::runtime_error(
            "Invalid password: must be at least 8 characters"));
    else if (len > 64)
        throw(std::runtime_error(
            "Invalid password: must be at most 64 characters"));
    for (size_t i = 0; i < len; ++i) {
        if (!std::isgraph(static_cast<unsigned char>(password[i])))
            throw(std::runtime_error(
                "Invalid password: "
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
    if (setsockopt(
        serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
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
    if (bind(serverSocket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        int errsv = errno;
        throw(std::runtime_error(createErrorMessage("bind", errsv)));
    }

    // Start listening for incoming connections
    // MAX_QUEUE of 8 means up to 8 pending connections can queue
    if (listen(serverSocket_, MAX_QUEUE) < 0) {
        int errsv = errno;
        throw(std::runtime_error(createErrorMessage("listen", errsv)));
    }
}
