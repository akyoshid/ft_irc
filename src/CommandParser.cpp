#include "CommandParser.hpp"

#include <algorithm>
#include <cctype>
#include <string>

#include "utils.hpp"

CommandParser::CommandParser() {}

CommandParser::~CommandParser() {}

// ==========================================
// Parser: RFC1459 message format
// Format: [:prefix] COMMAND [params] [:trailing]
// ==========================================

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
Command CommandParser::parseCommand(const std::string& message) {
  Command cmd;
  size_t pos = 0;

  // Skip leading spaces
  while (pos < message.length() && message[pos] == ' ') {
    ++pos;
  }

  // Parse optional prefix (messages from clients usually don't have prefix)
  if (pos < message.length() && message[pos] == ':') {
    ++pos;
    size_t spacePos = message.find(' ', pos);
    if (spacePos != std::string::npos) {
      cmd.prefix = message.substr(pos, spacePos - pos);
      pos = spacePos + 1;
    } else {
      // Malformed: prefix but no command
      return cmd;
    }
  }

  // Skip spaces before command
  while (pos < message.length() && message[pos] == ' ') {
    ++pos;
  }

  // Parse command
  size_t cmdEnd = pos;
  while (cmdEnd < message.length() && message[cmdEnd] != ' ') {
    ++cmdEnd;
  }

  if (cmdEnd == pos) {
    // No command found
    return cmd;
  }

  cmd.command = message.substr(pos, cmdEnd - pos);
  // Convert command to uppercase for case-insensitive matching
  for (size_t i = 0; i < cmd.command.length(); ++i) {
    cmd.command[i] = std::toupper(cmd.command[i]);
  }

  pos = cmdEnd;

  // Parse parameters
  while (pos < message.length()) {
    // Skip spaces
    while (pos < message.length() && message[pos] == ' ') {
      ++pos;
    }

    if (pos >= message.length()) {
      break;
    }

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
    pos = paramEnd;
  }

  return cmd;
}
