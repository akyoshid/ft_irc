/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: reasuke <reasuke@student.42tokyo.jp>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/03 07:47:24 by akyoshid          #+#    #+#             */
/*   Updated: 2025/11/08 18:02:52 by reasuke          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <signal.h>
#include <string>
#include <iostream>
#include <stdexcept>
#include "Server.hpp"
#include "utils.hpp"

volatile sig_atomic_t g_shutdown = 0;

namespace {
    void checkUsage(int argc) {
        if (argc != 3)
            throw(std::runtime_error(
                "Usage: ./ircserv <port> <password>"));
    }

    void signalHandler(int signum) {
        if (signum == SIGINT || signum == SIGTERM)
            g_shutdown = 1;
    }

    void setupSignalHandlers() {
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);
        signal(SIGPIPE, SIG_IGN);
    }
}  // namespace

int main(int argc, char *argv[]) {
    try {
        checkUsage(argc);
        setupSignalHandlers();
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
