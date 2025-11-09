#ifndef SRC_SERVER_COMMANDPARSER_HPP_
#define SRC_SERVER_COMMANDPARSER_HPP_

#include <string>

#include "Client.hpp"

// CommandParser: Parses and processes IRC commands
// Currently a stub implementation that only logs messages
// Future: Will implement PASS, NICK, USER, JOIN, PRIVMSG, etc.
class CommandParser {
 public:
  CommandParser();
  ~CommandParser();

  // Process a message from a client
  // Currently: Just logs the message
  // Future: Parse IRC commands and execute them
  void processMessage(Client* client, const std::string& message);

 private:
  CommandParser(const CommandParser& src);             // = delete
  CommandParser& operator=(const CommandParser& src);  // = delete
};

#endif
