/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   User.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: reasuke <reasuke@student.42tokyo.jp>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/04 00:29:00 by akyoshid          #+#    #+#             */
/*   Updated: 2025/11/09 00:00:00 by reasuke          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "User.hpp"

#include <unistd.h>

#include <set>
#include <string>

User::User(int socketFd, const std::string& ip)
    : socketFd_(socketFd), ip_(ip), authenticated_(false), registered_(false) {}

User::~User() {
  if (socketFd_ != INVALID_FD) {
    close(socketFd_);
  }
}

// Getters
int User::getSocketFd() const { return socketFd_; }

const std::string& User::getIp() const { return ip_; }

const std::string& User::getNickname() const { return nickname_; }

const std::string& User::getUsername() const { return username_; }

const std::string& User::getRealname() const { return realname_; }

bool User::isAuthenticated() const { return authenticated_; }

bool User::isRegistered() const { return registered_; }

// Setters
void User::setNickname(const std::string& nickname) { nickname_ = nickname; }

void User::setUsername(const std::string& username) { username_ = username; }

void User::setRealname(const std::string& realname) { realname_ = realname; }

void User::setAuthenticated(bool authenticated) {
  authenticated_ = authenticated;
}

void User::setRegistered(bool registered) { registered_ = registered; }

// Channel operations
void User::joinChannel(const std::string& channel) {
  joinedChannels_.insert(channel);
}

void User::leaveChannel(const std::string& channel) {
  joinedChannels_.erase(channel);
}

bool User::isInChannel(const std::string& channel) const {
  return joinedChannels_.find(channel) != joinedChannels_.end();
}

const std::set<std::string>& User::getJoinedChannels() const {
  return joinedChannels_;
}

// Buffer access
std::string& User::getReadBuffer() { return readBuffer_; }

std::string& User::getWriteBuffer() { return writeBuffer_; }
