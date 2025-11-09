/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akyoshid <akyoshid@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/03 23:52:25 by akyoshid          #+#    #+#             */
/*   Updated: 2025/11/08 04:06:26 by akyoshid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef INCLUDE_SERVER_HPP_
#define INCLUDE_SERVER_HPP_

#include <sys/epoll.h>

#include <string>

#include "server/CommandParser.hpp"
#include "server/ConnectionManager.hpp"
#include "server/EventLoop.hpp"

#define INVALID_FD -1
#define MAX_QUEUE 8
#define MAX_EVENTS 64

class Server {
 public:
  Server(const std::string& portStr, const std::string& password);
  ~Server();
  void run();

 private:
  int port_;
  std::string password_;
  int serverSocket_;
  EventLoop eventLoop_;
  ConnectionManager connManager_;
  CommandParser cmdParser_;

  // Helper methods
  void validateAndSetPort(const std::string& portStr);
  void validatePassword(const std::string& password);
  void setupServerSocket();
  void handleEvent(const struct epoll_event& event);
  void acceptConnections();
  void handleClientError(int fd);
  void handleClientRead(Client* client);
  void handleClientWrite(Client* client);
  void disconnectClient(int fd);

  Server();                              // = delete
  Server(const Server& src);             // = delete
  Server& operator=(const Server& src);  // = delete
};

#endif
