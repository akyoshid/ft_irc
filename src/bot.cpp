/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bot.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akyoshid <akyoshid@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/13 00:35:04 by akyoshid          #+#    #+#             */
/*   Updated: 2025/11/13 04:10:25 by akyoshid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <signal.h>

#include <exception>
#include <iostream>

#include "BotClient.hpp"
#include "utils.hpp"

volatile sig_atomic_t g_shutdown = 0;

namespace {

void signalHandler(int signum) {
  (void)signum;
  g_shutdown = 1;
}

void checkUsage(int argc) {
  if (argc != 6) {
    throw std::runtime_error(
        "Usage: ./ircbot <host> <port> <password> <nickname> <channel>");
  }
}

void setupSignalHandlers() {
  signal(SIGPIPE, SIG_IGN);
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);
}
}  // namespace

int main(int argc, char* argv[]) {
  try {
    checkUsage(argc);
    setupSignalHandlers();
    BotClient bot(argv[1], argv[2], argv[3], argv[4], argv[5]);
    bot.run();
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
    log(LOG_LEVEL_ERROR, LOG_CATEGORY_SYSTEM,
        "Bot stopped due to critical error");
    return 1;
  }
  log(LOG_LEVEL_INFO, LOG_CATEGORY_SYSTEM, "Bot stopped successfully");
  return 0;
}
