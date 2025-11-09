/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: reasuke <reasuke@student.42tokyo.jp>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/04 00:29:00 by akyoshid          #+#    #+#             */
/*   Updated: 2025/11/08 18:03:26 by reasuke          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"

#include <string>

Client::Client(int socketFd, const std::string& ip)
    : socketFd_(socketFd), ip_(ip), authenticated_(false), registered_(false) {}

Client::~Client() {}

// Getters
int Client::getSocketFd() const { return socketFd_; }

const std::string& Client::getIp() const { return ip_; }

const std::string& Client::getNickname() const { return nickname_; }

const std::string& Client::getUsername() const { return username_; }

bool Client::isAuthenticated() const { return authenticated_; }

bool Client::isRegistered() const { return registered_; }

// Setters
void Client::setNickname(const std::string& nickname) { nickname_ = nickname; }

void Client::setUsername(const std::string& username) { username_ = username; }

void Client::setAuthenticated(bool authenticated) {
  authenticated_ = authenticated;
}

void Client::setRegistered(bool registered) { registered_ = registered; }

// Buffer access
std::string& Client::getReadBuffer() { return readBuffer_; }

std::string& Client::getWriteBuffer() { return writeBuffer_; }
