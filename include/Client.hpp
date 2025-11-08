/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akyoshid <akyoshid@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/04 00:29:05 by akyoshid          #+#    #+#             */
/*   Updated: 2025/11/08 00:25:33 by akyoshid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef INCLUDE_CLIENT_HPP_
#define INCLUDE_CLIENT_HPP_

#include <string>

class Client {
 public:
  Client(int socketFd, const std::string& ip);
  ~Client();
  int socketFd_;
  std::string ip_;
  std::string nickname_;
  std::string username_;
  std::string readBuffer_;
  std::string writeBuffer_;
  bool authenticated_;
  bool registered_;

 private:
  Client();                              // = delete
  Client(const Client& src);             // = delete
  Client& operator=(const Client& src);  // = delete
};

#endif
