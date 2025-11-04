/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akyoshid <akyoshid@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/03 23:52:25 by akyoshid          #+#    #+#             */
/*   Updated: 2025/11/04 00:32:18 by akyoshid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef INCLUDE_SERVER_HPP_
# define INCLUDE_SERVER_HPP_

# include <string>

class Server {
 public:
    Server(const std::string& portStr, const std::string& password);
    ~Server();
 private:
    int port_;
    std::string password_;
    void setPort(const std::string& portStr);
    void setPassword(const std::string& password);
    Server();  // = delete
    Server(const Server& src);  // = delete
    Server& operator=(const Server& src);  // = delete
};

#endif
