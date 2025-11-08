/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akyoshid <akyoshid@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/04 14:29:49 by akyoshid          #+#    #+#             */
/*   Updated: 2025/11/08 04:37:42 by akyoshid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef INCLUDE_UTILS_HPP_
#define INCLUDE_UTILS_HPP_

#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"

#include <string>

enum LogLevel {
  LOG_LEVEL_INFO,
  LOG_LEVEL_WARNING,
  LOG_LEVEL_ERROR,
  LOG_LEVEL_DEBUG
};

enum LogCategory {
  LOG_CATEGORY_CONNECTION,
  LOG_CATEGORY_AUTH,
  LOG_CATEGORY_COMMAND,
  LOG_CATEGORY_CHANNEL,
  LOG_CATEGORY_PERMISSION,
  LOG_CATEGORY_NETWORK,
  LOG_CATEGORY_SYSTEM
};

std::string createLog(LogLevel level, LogCategory category,
                      const std::string& message);
void log(LogLevel level, LogCategory category, const std::string& message);
std::string createErrorMessage(const std::string& context, int errsv);
std::string int_to_string(int value);

#endif
