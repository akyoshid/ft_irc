/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akyoshid <akyoshid@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/04 00:29:00 by akyoshid          #+#    #+#             */
/*   Updated: 2025/11/08 00:26:42 by akyoshid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <string>
#include "../include/Client.hpp"

Client::Client(int socketFd, std::string ip)
    : socketFd_(socketFd), ip_(ip), authenticated_(false), registered_(false) {
}

Client::~Client() {
}
