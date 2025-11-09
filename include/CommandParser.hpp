#ifndef SRC_SERVER_COMMANDPARSER_HPP_
#define SRC_SERVER_COMMANDPARSER_HPP_

#include <string>

#include "User.hpp"

// CommandParser: Parses and processes IRC commands
// Currently a stub implementation that only logs messages
// Future: Will implement PASS, NICK, USER, JOIN, PRIVMSG, etc.
class CommandParser {
 public:
  CommandParser();
  ~CommandParser();

  // Process a message from a user
  // Currently: Just logs the message
  // Future: Parse IRC commands and execute them
  void processMessage(User* user, const std::string& message);

 private:
  CommandParser(const CommandParser& src);             // = delete
  CommandParser& operator=(const CommandParser& src);  // = delete
};

#endif
