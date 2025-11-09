/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   User.hpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akyoshid <akyoshid@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/04 00:29:05 by akyoshid          #+#    #+#             */
/*   Updated: 2025/11/09 00:00:00 by akyoshid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef INCLUDE_USER_HPP_
#define INCLUDE_USER_HPP_

#include <set>
#include <string>

#define INVALID_FD -1

class User {
 public:
  User(int socketFd, const std::string& ip);
  ~User();

  // Getters
  int getSocketFd() const;
  const std::string& getIp() const;
  const std::string& getNickname() const;
  const std::string& getUsername() const;
  const std::string& getRealname() const;
  bool isAuthenticated() const;
  bool isRegistered() const;

  // Setters
  void setNickname(const std::string& nickname);
  void setUsername(const std::string& username);
  void setRealname(const std::string& realname);
  void setAuthenticated(bool authenticated);
  void setRegistered(bool registered);

  // Channel operations
  void joinChannel(const std::string& channel);
  void leaveChannel(const std::string& channel);
  bool isInChannel(const std::string& channel) const;
  const std::set<std::string>& getJoinedChannels() const;

  // Buffer access (for ConnectionManager and Server)
  std::string& getReadBuffer();
  std::string& getWriteBuffer();

 private:
  int socketFd_;
  std::string ip_;
  std::string nickname_;
  std::string username_;
  std::string realname_;
  std::string readBuffer_;
  std::string writeBuffer_;
  bool authenticated_;
  bool registered_;
  std::set<std::string> joinedChannels_;

  User();                            // = delete
  User(const User& src);             // = delete
  User& operator=(const User& src);  // = delete
};

#endif
