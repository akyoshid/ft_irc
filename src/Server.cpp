/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akyoshid <akyoshid@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/03 23:57:17 by akyoshid          #+#    #+#             */
/*   Updated: 2025/11/04 00:27:23 by akyoshid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <string>
#include <cctype>
#include <stdexcept>
#include <iostream>
#include "../include/Server.hpp"

Server::Server(const std::string& portStr, const std::string& password) {
    setPort(portStr);
    setPassword(password);
    std::cout << "port: " << port_ << "\n";
    std::cout << "password: " << password_ << "\n";
}

Server::~Server() {
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
