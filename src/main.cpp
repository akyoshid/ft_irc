/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akyoshid <akyoshid@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/03 07:47:24 by akyoshid          #+#    #+#             */
/*   Updated: 2025/11/03 23:49:38 by akyoshid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <string>
#include <cctype>
#include <iostream>
#include <stdexcept>

namespace {

    // Validate and convert a port string to an int.
    // Only accepts exactly 4 digits
    //  and the range 6665-6669 (IRC standard ports).
    // No sign, no spaces.
    // Throws std::runtime_error on invalid input.
    int checkPortNum(const std::string& str) {
        if (str.length() != 4)
            throw(std::runtime_error(
                "Invalid port: must be exactly 4 digits (6665-6669)"));
        int port = 0;
        for (size_t i = 0; i < 4; ++i) {
            if (!std::isdigit(static_cast<unsigned char>(str[i])))
                throw(std::runtime_error(
                    "Invalid port: contains non-digit characters"));
            port = port * 10 + (str[i] - '0');
        }
        if (port < 6665 || port > 6669) {
            throw(std::runtime_error(
                "Invalid port: allowed range is 6665-6669"));
        } else {
            return port;
        }
    }

    // Validate password string and return it by reference.
    // Requirements: printable (no spaces), length between 8 and 64.
    // Throws std::runtime_error on invalid input.
    const std::string& checkPassword(const std::string& str) {
        size_t len = str.length();
        if (len < 8)
            throw(std::runtime_error(
                "Invalid password: must be at least 8 characters"));
        else if (len > 64)
            throw(std::runtime_error(
                "Invalid password: must be at most 64 characters"));
        for (size_t i = 0; i < len; ++i) {
            if (!std::isgraph(static_cast<unsigned char>(str[i])))
                throw(std::runtime_error(
                    "Invalid password: "
                    "contains non-printable or space characters"));
        }
        return str;
    }

    // Validates command line arguments and extracts port and password
    // Throws std::runtime_error on argument errors.
    void checkArgs(int argc, char *argv[], int* port, std::string* password) {
        if (argc != 3)
            throw(std::runtime_error(
                "Invalid arguments: Usage: ./ircserv <port> <password>"));
        *port = checkPortNum(argv[1]);
        *password = checkPassword(argv[2]);
    }

}  // namespace

int main(int argc, char *argv[]) {
    int port;
    std::string password;
    try {
        checkArgs(argc, argv, &port, &password);
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    std::cout << "port: " << port << "\n";
    std::cout << "password: " << password << "\n";
    return 0;
}
