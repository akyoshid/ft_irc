#ifndef SRC_SERVER_COMMANDPARSER_HPP_
#define SRC_SERVER_COMMANDPARSER_HPP_

#include <string>
#include <vector>

#include "User.hpp"

// Command: Represents a parsed IRC command
struct Command {
  std::string prefix;   // Optional prefix (usually empty from clients)
  std::string command;  // Command name (PASS, NICK, JOIN, etc.)
  std::vector<std::string> params;  // Command parameters

  Command() : prefix(""), command("") {}
};

// CommandParser: Parses IRC commands per RFC1459
class CommandParser {
 public:
  CommandParser();
  ~CommandParser();

  // Parse IRC message into Command structure
  // Format: [:prefix] COMMAND [params] [:trailing]
  // message: Raw IRC message (CRLF terminator already stripped)
  // Throws: std::runtime_error if message format is invalid
  Command parseCommand(const std::string& message);

 private:
  CommandParser(const CommandParser& src);             // = delete
  CommandParser& operator=(const CommandParser& src);  // = delete
};

#endif
