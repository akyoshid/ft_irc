/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akyoshid <akyoshid@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/04 14:26:18 by akyoshid          #+#    #+#             */
/*   Updated: 2025/11/07 18:54:12 by akyoshid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <time.h>
#include <iostream>
#include <string>
#include <cstring>
#include <cerrno>
#include <sstream>
#include "../include/utils.hpp"

// Category list
// - Connection
// - Auth
// - Command
// - Channel
// - Permission
// - Network
// - System
void log(
    LogLevel level, const std::string& category, const std::string& message) {

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
        default:
            color = RED;
            levelStr = "ERROR";
            break;
    }

    std::cout
        << "[" << timeStr << "] "
        << "[" << color << levelStr << RESET << "] "
        << "[" << category << "] "
        << message << std::endl;
}

std::string createErrorMessage(const std::string& context, int errsv) {
    return "Error in " + context + ": " + strerror(errsv);
}

std::string int_to_string(int value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}
