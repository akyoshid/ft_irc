/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akyoshid <akyoshid@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/03 07:47:24 by akyoshid          #+#    #+#             */
/*   Updated: 2025/11/08 04:07:11 by akyoshid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <string>
#include <iostream>
#include <stdexcept>
#include "../include/Server.hpp"
#include "../include/utils.hpp"

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
        server.run();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        log(LOG_LEVEL_INFO, LOG_CATEGORY_SYSTEM,
            "Server stopped due to critical error");
        return 1;
    }
    log(LOG_LEVEL_INFO, LOG_CATEGORY_SYSTEM,
        "Server stopped successfully");
    return 0;
}
