/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akyoshid <akyoshid@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/03 07:47:24 by akyoshid          #+#    #+#             */
/*   Updated: 2025/11/04 00:29:38 by akyoshid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <string>
#include <iostream>
#include <stdexcept>
#include "../include/Server.hpp"

namespace {
    void checkUsage(int argc) {
        if (argc != 3)
            throw(std::runtime_error(
                "Usage: ./ircserv <port> <password>"));
    }
}  // namespace

int main(int argc, char *argv[]) {
    try {
        checkUsage(argc);
        Server server(argv[1], argv[2]);
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
