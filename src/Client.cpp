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
