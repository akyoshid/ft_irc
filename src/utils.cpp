/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akyoshid <akyoshid@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/04 14:26:18 by akyoshid          #+#    #+#             */
/*   Updated: 2025/11/08 04:38:00 by akyoshid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/utils.hpp"

#include <time.h>

#include <cctype>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

std::string createLog(LogLevel level, LogCategory category,
                      const std::string& message) {
  time_t now = time(NULL);
  char timeStr[20];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));

  std::string color;
  std::string levelStr;
  switch (level) {
    case LOG_LEVEL_INFO:
      color = GREEN;
      levelStr = "INFO";
      break;
    case LOG_LEVEL_WARNING:
      color = YELLOW;
      levelStr = "WARN";
      break;
    case LOG_LEVEL_ERROR:
      color = RED;
      levelStr = "ERROR";
      break;
    case LOG_LEVEL_DEBUG:
    default:
      color = BLUE;
      levelStr = "DEBUG";
      break;
  }

  std::string categoryStr;
  switch (category) {
    case LOG_CATEGORY_CONNECTION:
      categoryStr = "Connection";
      break;
    case LOG_CATEGORY_AUTH:
      categoryStr = "Auth";
      break;
    case LOG_CATEGORY_COMMAND:
      categoryStr = "Command";
      break;
    case LOG_CATEGORY_CHANNEL:
      categoryStr = "Channel";
      break;
    case LOG_CATEGORY_PERMISSION:
      categoryStr = "Permission";
      break;
    case LOG_CATEGORY_NETWORK:
      categoryStr = "Network";
      break;
    case LOG_CATEGORY_SYSTEM:
    default:
      categoryStr = "System";
      break;
  }

  std::ostringstream oss;
  oss << "[" << timeStr << "] "
      << "[" << color << levelStr << RESET << "] "
      << "[" << categoryStr << "] " << message;
  return oss.str();
}

void log(LogLevel level, LogCategory category, const std::string& message) {
  std::cout << createLog(level, category, message) << std::endl;
}

std::string createErrorMessage(const std::string& context, int errsv) {
  return "Error in " + context + ": " + strerror(errsv);
}

std::string int_to_string(int value) {
  std::ostringstream oss;
  oss << value;
  return oss.str();
}

std::string normalizeNickname(const std::string& nickname) {
  std::string normalized;
  normalized.reserve(nickname.length());
  for (size_t i = 0; i < nickname.length(); ++i) {
    normalized += std::tolower(static_cast<unsigned char>(nickname[i]));
  }
  return normalized;
}

std::string normalizeChannelName(const std::string& channelName) {
  std::string normalized;
  normalized.reserve(channelName.length());
  for (size_t i = 0; i < channelName.length(); ++i) {
    normalized += std::tolower(static_cast<unsigned char>(channelName[i]));
  }
  return normalized;
}
