#include "CommandRouter.hpp"

#include <algorithm>
#include <cctype>

#include "utils.hpp"

CommandRouter::CommandRouter(UserManager* userMgr, ChannelManager* chanMgr,
                             const std::string& password)
    : userManager_(userMgr), channelManager_(chanMgr), password_(password) {}

CommandRouter::~CommandRouter() {}

// ==========================================
// Main entry point
// ==========================================

void CommandRouter::processMessage(User* user, const std::string& message) {
  if (!user) {
    log(LOG_LEVEL_WARNING, LOG_CATEGORY_COMMAND,
        "processMessage called with NULL user");
    return;
  }

  if (message.empty()) {
    return;
  }

  log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND, user->getIp() + ": " + message);

  Command cmd = parseCommand(message);
  if (cmd.command.empty()) {
    log(LOG_LEVEL_WARNING, LOG_CATEGORY_COMMAND,
        "Failed to parse command: " + message);
    return;
  }

  dispatch(user, cmd);
}

// ==========================================
// Parser: RFC1459 message format
// Format: [:prefix] COMMAND [params] [:trailing]
// ==========================================

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
Command CommandRouter::parseCommand(const std::string& message) {
  Command cmd;
  const std::string& msg = message;
  size_t pos = 0;

  // Skip leading spaces
  while (pos < msg.length() && msg[pos] == ' ') {
    ++pos;
  }

  // Parse optional prefix (messages from clients usually don't have prefix)
  if (pos < msg.length() && msg[pos] == ':') {
    ++pos;
    size_t spacePos = msg.find(' ', pos);
    if (spacePos != std::string::npos) {
      cmd.prefix = msg.substr(pos, spacePos - pos);
      pos = spacePos + 1;
    } else {
      // Malformed: prefix but no command
      return cmd;
    }
  }

  // Skip spaces before command
  while (pos < msg.length() && msg[pos] == ' ') {
    ++pos;
  }

  // Parse command
  size_t cmdEnd = pos;
  while (cmdEnd < msg.length() && msg[cmdEnd] != ' ') {
    ++cmdEnd;
  }

  if (cmdEnd == pos) {
    // No command found
    return cmd;
  }

  cmd.command = msg.substr(pos, cmdEnd - pos);
  // Convert command to uppercase for case-insensitive matching
  for (size_t i = 0; i < cmd.command.length(); ++i) {
    cmd.command[i] = std::toupper(cmd.command[i]);
  }

  pos = cmdEnd;

  // Parse parameters
  while (pos < msg.length()) {
    // Skip spaces
    while (pos < msg.length() && msg[pos] == ' ') {
      ++pos;
    }

    if (pos >= msg.length()) {
      break;
    }

    // Trailing parameter (starts with ':')
    if (msg[pos] == ':') {
      ++pos;
      cmd.params.push_back(msg.substr(pos));
      break;
    }

    // Regular parameter
    size_t paramEnd = pos;
    while (paramEnd < msg.length() && msg[paramEnd] != ' ') {
      ++paramEnd;
    }

    cmd.params.push_back(msg.substr(pos, paramEnd - pos));
    pos = paramEnd;
  }

  return cmd;
}

// ==========================================
// Dispatcher
// ==========================================

void CommandRouter::dispatch(User* user, const Command& cmd) {
  if (cmd.command == "PASS") {
    handlePass(user, cmd);
  } else if (cmd.command == "NICK") {
    handleNick(user, cmd);
  } else if (cmd.command == "USER") {
    handleUser(user, cmd);
  } else if (cmd.command == "JOIN") {
    handleJoin(user, cmd);
  } else if (cmd.command == "PART") {
    handlePart(user, cmd);
  } else if (cmd.command == "PRIVMSG") {
    handlePrivmsg(user, cmd);
  } else if (cmd.command == "KICK") {
    handleKick(user, cmd);
  } else if (cmd.command == "INVITE") {
    handleInvite(user, cmd);
  } else if (cmd.command == "TOPIC") {
    handleTopic(user, cmd);
  } else if (cmd.command == "MODE") {
    handleMode(user, cmd);
  } else if (cmd.command == "QUIT") {
    handleQuit(user, cmd);
  } else {
    log(LOG_LEVEL_WARNING, LOG_CATEGORY_COMMAND,
        "Unknown command: " + cmd.command);
    sendResponse(user, ResponseFormatter::errUnknownCommand(cmd.command));
  }
}

// ==========================================
// Command handlers (stub implementations)
// ==========================================

void CommandRouter::handlePass(User* user, const Command& cmd) {
  // Check if user is already registered
  if (user->isRegistered()) {
    sendResponse(user, ResponseFormatter::errAlreadyRegistered());
    return;
  }

  // Check parameter count
  if (cmd.params.empty()) {
    sendResponse(user, ResponseFormatter::errNeedMoreParams("PASS"));
    return;
  }

  // Verify password
  if (cmd.params[0] != password_) {
    log(LOG_LEVEL_WARNING, LOG_CATEGORY_COMMAND,
        "Authentication failed for " + user->getIp() + ": incorrect password");
    sendResponse(user, ResponseFormatter::errPasswdMismatch());
    return;
  }

  // Set authenticated flag
  user->setAuthenticated(true);
  log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND,
      "Authentication successful for " + user->getIp());
}

void CommandRouter::handleNick(User* user, const Command& cmd) {
  // Check parameter count
  if (cmd.params.empty()) {
    sendResponse(user, ResponseFormatter::errNeedMoreParams("NICK"));
    return;
  }

  const std::string& newNick = cmd.params[0];

  // Validate nickname format
  if (!isValidNickname(newNick)) {
    sendResponse(user, ResponseFormatter::errErroneusNickname(newNick));
    return;
  }

  // Check if nickname is already in use by another user
  if (userManager_->isNicknameInUse(newNick)) {
    sendResponse(user, ResponseFormatter::errNicknameInUse(newNick));
    return;
  }

  // Update nickname
  std::string oldNick = user->getNickname();
  userManager_->updateNickname(user, oldNick, newNick);

  log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND,
      "Nickname set: " + user->getIp() + " -> " + newNick);

  // Check if registration is complete (authenticated + nickname + user info)
  if (!user->isRegistered() && user->isAuthenticated() &&
      !user->getUsername().empty()) {
    // Complete registration
    user->setRegistered(true);

    // Send welcome messages (001-004)
    sendResponse(user, ResponseFormatter::rplWelcome(user));
    sendResponse(user, ResponseFormatter::rplYourHost(user));
    sendResponse(user, ResponseFormatter::rplCreated(user));
    sendResponse(user, ResponseFormatter::rplMyInfo(user));

    log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND,
        "Registration complete: " + user->getNickname() + "!" +
            user->getUsername() + "@" + user->getIp());
  }
}

void CommandRouter::handleUser(User* user, const Command& cmd) {
  // Check if user is already registered
  if (user->isRegistered()) {
    sendResponse(user, ResponseFormatter::errAlreadyRegistered());
    return;
  }

  // Check parameter count (username, hostname, servername, realname)
  if (cmd.params.size() < 4) {
    sendResponse(user, ResponseFormatter::errNeedMoreParams("USER"));
    return;
  }

  // Set user information
  const std::string& username = cmd.params[0];
  const std::string& realname = cmd.params[3];

  user->setUsername(username);
  user->setRealname(realname);

  log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND,
      "User info set: " + user->getIp() + " (username: " + username + ")");

  // Check if registration is complete (authenticated + nickname + user info)
  if (user->isAuthenticated() && !user->getNickname().empty()) {
    // Complete registration
    user->setRegistered(true);

    // Send welcome messages (001-004)
    sendResponse(user, ResponseFormatter::rplWelcome(user));
    sendResponse(user, ResponseFormatter::rplYourHost(user));
    sendResponse(user, ResponseFormatter::rplCreated(user));
    sendResponse(user, ResponseFormatter::rplMyInfo(user));

    log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND,
        "Registration complete: " + user->getNickname() + "!" +
            user->getUsername() + "@" + user->getIp());
  }
}

void CommandRouter::handleJoin(User* user, const Command& cmd) {
  log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND,
      "JOIN command received (stub) from: " + user->getNickname());
  if (cmd.params.empty()) {
    sendResponse(user, ResponseFormatter::errNeedMoreParams("JOIN"));
    return;
  }
  // TODO(Phase 4): Implement channel joining
}

void CommandRouter::handlePart(User* user, const Command& cmd) {
  log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND,
      "PART command received (stub) from: " + user->getNickname());
  if (cmd.params.empty()) {
    sendResponse(user, ResponseFormatter::errNeedMoreParams("PART"));
    return;
  }
  // TODO(Phase 4): Implement channel leaving
}

void CommandRouter::handlePrivmsg(User* user, const Command& cmd) {
  log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND,
      "PRIVMSG command received (stub) from: " + user->getNickname());
  if (cmd.params.size() < 2) {
    sendResponse(user, ResponseFormatter::errNeedMoreParams("PRIVMSG"));
    return;
  }
  // TODO(Phase 4): Implement message sending
}

void CommandRouter::handleKick(User* user, const Command& cmd) {
  log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND,
      "KICK command received (stub) from: " + user->getNickname());
  if (cmd.params.size() < 2) {
    sendResponse(user, ResponseFormatter::errNeedMoreParams("KICK"));
    return;
  }
  // TODO(Phase 5): Implement kick functionality
}

void CommandRouter::handleInvite(User* user, const Command& cmd) {
  log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND,
      "INVITE command received (stub) from: " + user->getNickname());
  if (cmd.params.size() < 2) {
    sendResponse(user, ResponseFormatter::errNeedMoreParams("INVITE"));
    return;
  }
  // TODO(Phase 5): Implement invite functionality
}

void CommandRouter::handleTopic(User* user, const Command& cmd) {
  log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND,
      "TOPIC command received (stub) from: " + user->getNickname());
  if (cmd.params.empty()) {
    sendResponse(user, ResponseFormatter::errNeedMoreParams("TOPIC"));
    return;
  }
  // TODO(Phase 5): Implement topic functionality
}

void CommandRouter::handleMode(User* user, const Command& cmd) {
  log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND,
      "MODE command received (stub) from: " + user->getNickname());
  if (cmd.params.empty()) {
    sendResponse(user, ResponseFormatter::errNeedMoreParams("MODE"));
    return;
  }
  // TODO(Phase 5): Implement mode functionality
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void CommandRouter::handleQuit(User* user, const Command& cmd) {
  std::string reason = cmd.params.empty() ? "Client quit" : cmd.params[0];
  log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND,
      "QUIT command received from: " + user->getNickname() + " (" + reason +
          ")");
  // TODO(Phase 4): Implement graceful disconnect
}

// ==========================================
// Helpers
// ==========================================

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void CommandRouter::sendResponse(User* user, const std::string& response) {
  if (!user) return;
  user->getWriteBuffer() += response;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
bool CommandRouter::isValidChannelName(const std::string& name) {
  if (name.empty() || name.length() > 200) {
    return false;
  }

  // Channel names must start with # or &
  if (name[0] != '#' && name[0] != '&') {
    return false;
  }

  // Channel names cannot contain spaces, commas, or control characters
  for (size_t i = 0; i < name.length(); ++i) {
    char c = name[i];
    if (c == ' ' || c == ',' || c == 7) {  // 7 = BELL
      return false;
    }
  }

  return true;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
bool CommandRouter::isValidNickname(const std::string& nickname) {
  if (nickname.empty() || nickname.length() > 9) {
    return false;
  }

  // First character must be a letter
  if (!std::isalpha(nickname[0])) {
    return false;
  }

  // Rest can be letters, digits, or special characters (-, [, ], \, `, ^, {, })
  for (size_t i = 1; i < nickname.length(); ++i) {
    char c = nickname[i];
    if (!std::isalnum(c) && c != '-' && c != '[' && c != ']' && c != '\\' &&
        c != '`' && c != '^' && c != '{' && c != '}') {
      return false;
    }
  }

  return true;
}
