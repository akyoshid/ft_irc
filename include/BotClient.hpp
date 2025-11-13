/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   BotClient.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akyoshid <akyoshid@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/13 00:34:52 by akyoshid          #+#    #+#             */
/*   Updated: 2025/11/13 01:14:29 by akyoshid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef INCLUDE_BOTCLIENT_HPP_
#define INCLUDE_BOTCLIENT_HPP_

#include <string>
#include <vector>

#include "EventLoop.hpp"

class BotClient {
 public:
  BotClient(const std::string& host, const std::string& port,
            const std::string& password, const std::string& nickname,
            const std::string& channel);
  ~BotClient();

  void run();

 private:
  void connectToServer();
  int createNonBlockingSocket() const;
  void setupEventLoop();
  void handleEvents();
  void handleRead();
  void handleWrite();
  void processMessage(const std::string& line);
  void processCommand(const std::string& prefix, const std::string& command,
                      const std::vector<std::string>& params,
                      const std::string& trailing);
  void handlePrivMsg(const std::string& prefix,
                     const std::vector<std::string>& params,
                     const std::string& trailing);
  void enqueueMessage(const std::string& message);
  void sendInitialMessages();
  void joinChannelIfNeeded();
  void respondToUser(const std::string& target, const std::string& command,
                     const std::string& senderNick);
  std::string extractNickname(const std::string& prefix) const;
  std::string currentTimeString() const;
  void closeSocket();

  std::string host_;
  std::string port_;
  std::string password_;
  std::string nickname_;
  std::string channel_;

  EventLoop eventLoop_;
  int socketFd_;
  std::string readBuffer_;
  std::string writeBuffer_;
  bool running_;
  bool registered_;
  bool joined_;
};

#endif
