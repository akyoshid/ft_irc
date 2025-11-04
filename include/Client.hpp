/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akyoshid <akyoshid@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/04 00:29:05 by akyoshid          #+#    #+#             */
/*   Updated: 2025/11/04 00:34:43 by akyoshid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef INCLUDE_CLIENT_HPP_
# define INCLUDE_CLIENT_HPP_

class Client {
 public:
    Client();
    ~Client();
 private:
    Client(const Client& src);  // = delete
    Client& operator=(const Client& src);  // = delete
};

#endif
