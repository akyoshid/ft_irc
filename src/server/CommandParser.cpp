#include "server/CommandParser.hpp"

#include <string>

#include "Client.hpp"
#include "utils.hpp"

CommandParser::CommandParser() {}

CommandParser::~CommandParser() {}

void CommandParser::processMessage(Client* client, const std::string& message) {
  // Stub implementation: Just log the message
  log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND, client->getIp() + ": " + message);

  // TODO: Future implementation will:
  // 1. Parse the IRC command (PASS, NICK, USER, JOIN, etc.)
  // 2. Validate command syntax
  // 3. Check client authentication/registration status
  // 4. Execute the appropriate command handler
  // 5. Send response to client

  // Example future structure:
  // Command cmd = parse(message);
  // if (!validate(cmd, client)) return sendError(client, ...);
  // execute(cmd, client);
}
