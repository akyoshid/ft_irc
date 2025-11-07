/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akyoshid <akyoshid@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/04 14:26:18 by akyoshid          #+#    #+#             */
/*   Updated: 2025/11/07 15:42:08 by akyoshid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <string>
#include <cstring>
#include <cerrno>
#include "../include/utils.hpp"

std::string createErrorMessage(const std::string& context, int errsv) {
    return "Error in " + context + ": " + strerror(errsv);
}

