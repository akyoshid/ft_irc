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
# define INCLUDE_SERVER_HPP_

# include <sys/epoll.h>
# include <string>
# include <map>
# include "Client.hpp"

# define INVALID_FD -1
# define MAX_QUEUE 8
# define MAX_CLIENTS 128
# define MAX_EVENTS 64
# define BUFFER_SIZE 4096
# define MAX_BUFFER_SIZE 8192

class Server {
 public:
    Server(const std::string& portStr, const std::string& password);
    ~Server();
    void run();

 private:
    int port_;
    std::string password_;
    int serverSocket_;
    int epollFd_;
    struct epoll_event events_[MAX_EVENTS];
    std::map<int, Client*> clients_;
    void setPort(const std::string& portStr);
    void setPassword(const std::string& password);
    void setServerSocket();
    void acceptNewConnection();
    void disconnectClient(int clientFd);
    void addToEpoll(int fd, uint32_t events);
    void modifyEpollEvents(int fd, uint32_t events);
    void removeFromEpoll(int fd);
    void recvFromClient(Client* client);
    void sendToClient(Client* client);
    void processMessage(Client* client, const std::string& message);
    Server();  // = delete
    Server(const Server& src);  // = delete
    Server& operator=(const Server& src);  // = delete
};

#endif
