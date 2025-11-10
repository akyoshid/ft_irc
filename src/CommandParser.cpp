#include "CommandParser.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <vector>

#include "utils.hpp"

CommandParser::CommandParser() {}

CommandParser::~CommandParser() {}

// ==========================================
// Parser: RFC1459 message format
// Format: [:prefix] COMMAND [params] [:trailing]
// ==========================================

// Helper: Skip spaces and return new position
static size_t skipSpaces(const std::string& str, size_t pos) {
  while (pos < str.length() && str[pos] == ' ') {
    ++pos;
  }
  return pos;
}

// Helper: Validate command format (letters or 3 digits)
static void validateCommand(const std::string& command) {
  if (command.empty()) {
    throw std::runtime_error("Invalid message: No command found.");
  }

  // Check for 3-digit numeric command (e.g., "001")
  if (command.length() == 3 &&
      std::isdigit(static_cast<unsigned char>(command[0])) &&
      std::isdigit(static_cast<unsigned char>(command[1])) &&
      std::isdigit(static_cast<unsigned char>(command[2]))) {
    return;
  }

  // Check for alphabetic command
  for (size_t i = 0; i < command.length(); ++i) {
    if (!std::isalpha(static_cast<unsigned char>(command[i]))) {
      throw std::runtime_error(
          "Invalid message: Command must be <letter> { <letter> } | <number> "
          "<number> <number>.");
    }
  }
}

// Helper: Validate parameters
static void validateParams(const std::vector<std::string>& params) {
  if (params.size() > 15) {
    throw std::runtime_error("Invalid message: Too many params (max 15).");
  }

  for (size_t i = 0; i < params.size(); ++i) {
    const std::string& param = params[i];
    if (param.find('\0') != std::string::npos) {
      throw std::runtime_error("Invalid message: Parameter contains NUL.");
    }
    if (param.find('\r') != std::string::npos) {
      throw std::runtime_error("Invalid message: Parameter contains CR.");
    }
    if (param.find('\n') != std::string::npos) {
      throw std::runtime_error("Invalid message: Parameter contains LF.");
    }
  }
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
Command CommandParser::parseCommand(const std::string& message) {
  Command cmd;
  size_t pos = 0;

  // Validate message not empty
  if (message.empty()) {
    throw std::runtime_error("Invalid message: Message is empty.");
  }

  // Validate message length (510 chars max per RFC1459)
  if (message.length() > 510) {
    throw std::runtime_error(
        "Invalid message: 510 characters maximum allowed for the command and "
        "its parameters.");
  }

  // Validate message doesn't start with space
  if (message[0] == ' ') {
    throw std::runtime_error(
        "Invalid message: Message must not start with space.");
  }

  // Parse optional prefix (messages from clients usually don't have prefix)
  if (message[pos] == ':') {
    ++pos;
    size_t spacePos = message.find(' ', pos);
    if (spacePos == std::string::npos) {
      throw std::runtime_error("Invalid message: Prefix found but no command.");
    }
    cmd.prefix = message.substr(pos, spacePos - pos);
    if (cmd.prefix.empty()) {
      throw std::runtime_error(
          "Invalid message: Prefix must not be empty or start with space.");
    }
    pos = skipSpaces(message, spacePos);
  }

  // Validate command exists
  if (pos >= message.length()) {
    throw std::runtime_error("Invalid message: No command found.");
  }

  // Parse command
  size_t cmdEnd = pos;
  while (cmdEnd < message.length() && message[cmdEnd] != ' ') {
    ++cmdEnd;
  }

  cmd.command = message.substr(pos, cmdEnd - pos);
  validateCommand(cmd.command);

  // Convert command to uppercase for case-insensitive matching
  for (size_t i = 0; i < cmd.command.length(); ++i) {
    cmd.command[i] = std::toupper(static_cast<unsigned char>(cmd.command[i]));
  }

  pos = skipSpaces(message, cmdEnd);

  // Parse parameters
  while (pos < message.length()) {
    // Trailing parameter (starts with ':')
    if (message[pos] == ':') {
      ++pos;
      cmd.params.push_back(message.substr(pos));
      break;
    }

    // Regular parameter
    size_t paramEnd = pos;
    while (paramEnd < message.length() && message[paramEnd] != ' ') {
      ++paramEnd;
    }

    cmd.params.push_back(message.substr(pos, paramEnd - pos));
    pos = skipSpaces(message, paramEnd);
  }

  // Validate parameters
  validateParams(cmd.params);

  return cmd;
}
