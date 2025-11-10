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
  // SECURITY NOTES:
  // 1. String comparison is not constant-time (timing attack vulnerability)
  // 2. No rate limiting on failed attempts (brute force vulnerability)
  // 3. No input sanitization (accepts control characters)
  // These are acceptable for educational purposes but should be addressed
  // in production systems
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
  // NOTE: Nickname comparison is case-insensitive per RFC1459
  // UserManager normalizes nicknames internally
  // NOTE: This check-then-set pattern is safe in single-threaded context
  // Multi-threaded implementation would require atomic check-and-set
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
    completeRegistration(user);
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
    completeRegistration(user);
  }
}

void CommandRouter::handleJoin(User* user, const Command& cmd) {
  // Check if user is registered
  if (!user->isRegistered()) {
    return;  // Silently ignore commands from unregistered users
  }

  // Check parameter count
  if (cmd.params.empty()) {
    sendResponse(user, ResponseFormatter::errNeedMoreParams("JOIN"));
    return;
  }

  const std::string& channelName = cmd.params[0];

  // Validate channel name
  // NOTE: Channel names are case-insensitive per RFC1459
  // ChannelManager normalizes names internally
  if (!isValidChannelName(channelName)) {
    sendResponse(user, ResponseFormatter::errNoSuchChannel(channelName));
    return;
  }

  // Get or create channel
  Channel* channel = channelManager_->getChannel(channelName);
  if (!channel) {
    channel = channelManager_->createChannel(channelName);
    if (!channel) {
      log(LOG_LEVEL_ERROR, LOG_CATEGORY_CHANNEL,
          "Failed to create channel: " + channelName);
      return;
    }
    // First user becomes operator
    channel->addOperator(user->getSocketFd());
    log(LOG_LEVEL_INFO, LOG_CATEGORY_CHANNEL,
        "Channel created: " + channelName + " by " + user->getNickname());
  }

  // Check if already in channel
  if (channel->isMember(user->getSocketFd())) {
    return;  // Already in channel, silently ignore
  }

  // Check channel modes
  if (channel->isInviteOnly() && !channel->isInvited(user->getSocketFd())) {
    sendResponse(user, ResponseFormatter::errInviteOnlyChan(channelName));
    return;
  }

  if (channel->hasUserLimit() &&
      channel->getMemberCount() >= channel->getUserLimit()) {
    sendResponse(user, ResponseFormatter::errChannelIsFull(channelName));
    return;
  }

  // Check channel key if set
  if (!channel->getKey().empty()) {
    std::string providedKey = cmd.params.size() > 1 ? cmd.params[1] : "";
    if (providedKey != channel->getKey()) {
      sendResponse(user, ResponseFormatter::errBadChannelKey(channelName));
      return;
    }
  }

  // Add user to channel
  channel->addMember(user->getSocketFd());
  user->joinChannel(channelName);

  // Remove invite if present
  if (channel->isInvited(user->getSocketFd())) {
    channel->removeInvite(user->getSocketFd());
  }

  // Broadcast JOIN to all channel members (including the user)
  std::string joinMsg = ResponseFormatter::rplJoin(user, channelName);
  const std::set<int>& members = channel->getMembers();
  for (std::set<int>::const_iterator it = members.begin(); it != members.end();
       ++it) {
    User* member = userManager_->getUserByFd(*it);
    if (member) {
      sendResponse(member, joinMsg);
    }
  }

  log(LOG_LEVEL_INFO, LOG_CATEGORY_CHANNEL,
      user->getNickname() + " joined " + channelName);
}

void CommandRouter::handlePart(User* user, const Command& cmd) {
  // Check if user is registered
  if (!user->isRegistered()) {
    return;  // Silently ignore commands from unregistered users
  }

  // Check parameter count
  if (cmd.params.empty()) {
    sendResponse(user, ResponseFormatter::errNeedMoreParams("PART"));
    return;
  }

  const std::string& channelName = cmd.params[0];
  std::string reason = cmd.params.size() > 1 ? cmd.params[1] : "";

  // Get channel
  Channel* channel = channelManager_->getChannel(channelName);
  if (!channel) {
    sendResponse(user, ResponseFormatter::errNoSuchChannel(channelName));
    return;
  }

  // Check if user is in channel
  if (!channel->isMember(user->getSocketFd())) {
    sendResponse(user, ResponseFormatter::errNotOnChannel(channelName));
    return;
  }

  // Broadcast PART to all channel members (including the user)
  std::string partMsg = ResponseFormatter::rplPart(user, channelName, reason);
  const std::set<int>& members = channel->getMembers();
  for (std::set<int>::const_iterator it = members.begin(); it != members.end();
       ++it) {
    User* member = userManager_->getUserByFd(*it);
    if (member) {
      sendResponse(member, partMsg);
    }
  }

  // Remove user from channel
  channel->removeMember(user->getSocketFd());
  channel->removeOperator(user->getSocketFd());
  user->leaveChannel(channelName);

  log(LOG_LEVEL_INFO, LOG_CATEGORY_CHANNEL,
      user->getNickname() + " left " + channelName);

  // Remove channel if empty
  if (channel->getMemberCount() == 0) {
    channelManager_->removeChannel(channelName);
    log(LOG_LEVEL_INFO, LOG_CATEGORY_CHANNEL,
        "Channel removed: " + channelName + " (empty)");
  }
}

void CommandRouter::handlePrivmsg(User* user, const Command& cmd) {
  // Check if user is registered
  if (!user->isRegistered()) {
    return;  // Silently ignore commands from unregistered users
  }

  // Check parameter count
  if (cmd.params.size() < 2) {
    sendResponse(user, ResponseFormatter::errNeedMoreParams("PRIVMSG"));
    return;
  }

  const std::string& target = cmd.params[0];
  const std::string& message = cmd.params[1];

  // Check if target is a channel or user
  if (!target.empty() && (target[0] == '#' || target[0] == '&')) {
    // Channel message
    Channel* channel = channelManager_->getChannel(target);
    if (!channel) {
      sendResponse(user, ResponseFormatter::errNoSuchChannel(target));
      return;
    }

    // Check if user is in channel
    if (!channel->isMember(user->getSocketFd())) {
      sendResponse(user, ResponseFormatter::errCannotSendToChan(target));
      return;
    }

    // TODO(Phase 5): Check moderated mode (+m) - only ops/voiced users can send
    // TODO(Phase 5): Check no-external messages (+n) - handled by membership
    // check above

    // Broadcast message to all channel members except sender
    std::string privmsgMsg =
        ResponseFormatter::rplPrivmsg(user, target, message);
    const std::set<int>& members = channel->getMembers();
    for (std::set<int>::const_iterator it = members.begin();
         it != members.end(); ++it) {
      if (*it != user->getSocketFd()) {  // Don't echo to sender
        User* member = userManager_->getUserByFd(*it);
        if (member) {
          sendResponse(member, privmsgMsg);
        }
      }
    }

    log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND,
        user->getNickname() + " sent message to " + target);
  } else {
    // Private message to user
    User* targetUser = userManager_->getUserByNickname(target);
    if (!targetUser) {
      sendResponse(user, ResponseFormatter::errNoSuchNick(target));
      return;
    }

    std::string privmsgMsg =
        ResponseFormatter::rplPrivmsg(user, target, message);
    sendResponse(targetUser, privmsgMsg);

    log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND,
        user->getNickname() + " sent private message to " + target);
  }
}

void CommandRouter::handleKick(User* user, const Command& cmd) {
  // Check if user is registered
  if (!user->isRegistered()) {
    return;  // Silently ignore commands from unregistered users
  }

  // KICK <channel> <user> [:<reason>]
  if (cmd.params.size() < 2) {
    sendResponse(user, ResponseFormatter::errNeedMoreParams("KICK"));
    return;
  }

  const std::string& channel = cmd.params[0];
  const std::string& targetNick = cmd.params[1];
  std::string reason = "Kicked";
  if (cmd.params.size() >= 3) {
    reason = cmd.params[2];
  }

  // Check if channel exists
  Channel* chan = channelManager_->getChannel(channel);
  if (!chan) {
    sendResponse(user, ResponseFormatter::errNoSuchChannel(channel));
    return;
  }

  // Check if kicker is on the channel
  if (!chan->isMember(user->getSocketFd())) {
    sendResponse(user, ResponseFormatter::errNotOnChannel(channel));
    return;
  }

  // Check if kicker is an operator
  if (!chan->isOperator(user->getSocketFd())) {
    sendResponse(user, ResponseFormatter::errChanOPrivsNeeded(channel));
    return;
  }

  // Check if target user exists
  User* targetUser = userManager_->getUserByNickname(targetNick);
  if (!targetUser) {
    sendResponse(user, ResponseFormatter::errNoSuchNick(targetNick));
    return;
  }

  // Check if target is on the channel
  if (!chan->isMember(targetUser->getSocketFd())) {
    sendResponse(user, ResponseFormatter::errUserNotInChannel(targetNick, channel));
    return;
  }

  // Broadcast KICK message to all channel members
  std::string kickMsg = ResponseFormatter::rplKick(user, channel, targetNick, reason);
  const std::set<int>& members = chan->getMembers();
  for (std::set<int>::const_iterator it = members.begin();
       it != members.end(); ++it) {
    User* member = userManager_->getUserByFd(*it);
    if (member) {
      sendResponse(member, kickMsg);
    }
  }

  // Remove target from channel
  chan->removeMember(targetUser->getSocketFd());
  targetUser->leaveChannel(channel);

  // If channel is empty, remove it
  if (chan->getMemberCount() == 0) {
    channelManager_->removeChannel(channel);
  }

  log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND,
      user->getNickname() + " kicked " + targetNick + " from " + channel);
}

void CommandRouter::handleInvite(User* user, const Command& cmd) {
  // Check if user is registered
  if (!user->isRegistered()) {
    return;  // Silently ignore commands from unregistered users
  }

  // INVITE <nickname> <channel>
  if (cmd.params.size() < 2) {
    sendResponse(user, ResponseFormatter::errNeedMoreParams("INVITE"));
    return;
  }

  const std::string& targetNick = cmd.params[0];
  const std::string& channel = cmd.params[1];

  // Check if target user exists
  User* targetUser = userManager_->getUserByNickname(targetNick);
  if (!targetUser) {
    sendResponse(user, ResponseFormatter::errNoSuchNick(targetNick));
    return;
  }

  // Check if channel exists
  Channel* chan = channelManager_->getChannel(channel);
  if (!chan) {
    sendResponse(user, ResponseFormatter::errNoSuchChannel(channel));
    return;
  }

  // Check if inviter is on the channel
  if (!chan->isMember(user->getSocketFd())) {
    sendResponse(user, ResponseFormatter::errNotOnChannel(channel));
    return;
  }

  // Check if target is already on the channel
  if (chan->isMember(targetUser->getSocketFd())) {
    sendResponse(user, ResponseFormatter::errUserOnChannel(targetNick, channel));
    return;
  }

  // If channel is invite-only, only operators can invite
  if (chan->isInviteOnly() && !chan->isOperator(user->getSocketFd())) {
    sendResponse(user, ResponseFormatter::errChanOPrivsNeeded(channel));
    return;
  }

  // Add target to invite list
  chan->addInvite(targetUser->getSocketFd());

  // Send confirmation to inviter (341 RPL_INVITING)
  sendResponse(user, ResponseFormatter::rplInviting(channel, targetNick));

  // Send INVITE message to target
  std::string inviteMsg = ResponseFormatter::rplInvite(user, targetNick, channel);
  sendResponse(targetUser, inviteMsg);

  log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND,
      user->getNickname() + " invited " + targetNick + " to " + channel);
}

void CommandRouter::handleTopic(User* user, const Command& cmd) {
  // Check if user is registered
  if (!user->isRegistered()) {
    return;  // Silently ignore commands from unregistered users
  }

  // TOPIC <channel> [:<topic>]
  if (cmd.params.empty()) {
    sendResponse(user, ResponseFormatter::errNeedMoreParams("TOPIC"));
    return;
  }

  const std::string& channel = cmd.params[0];

  // Check if channel exists
  Channel* chan = channelManager_->getChannel(channel);
  if (!chan) {
    sendResponse(user, ResponseFormatter::errNoSuchChannel(channel));
    return;
  }

  // Check if user is on the channel
  if (!chan->isMember(user->getSocketFd())) {
    sendResponse(user, ResponseFormatter::errNotOnChannel(channel));
    return;
  }

  // If no topic parameter, return current topic
  if (cmd.params.size() == 1) {
    const std::string& currentTopic = chan->getTopic();
    if (currentTopic.empty()) {
      sendResponse(user, ResponseFormatter::rplNoTopic(channel));
    } else {
      sendResponse(user, ResponseFormatter::rplTopic(channel, currentTopic));
    }
    return;
  }

  // Setting topic - check permissions
  if (chan->isTopicRestricted() && !chan->isOperator(user->getSocketFd())) {
    sendResponse(user, ResponseFormatter::errChanOPrivsNeeded(channel));
    return;
  }

  // Set new topic
  const std::string& newTopic = cmd.params[1];
  chan->setTopic(newTopic);

  // Broadcast topic change to all channel members
  std::string topicMsg = ResponseFormatter::rplTopicChange(user, channel, newTopic);
  const std::set<int>& members = chan->getMembers();
  for (std::set<int>::const_iterator it = members.begin();
       it != members.end(); ++it) {
    User* member = userManager_->getUserByFd(*it);
    if (member) {
      sendResponse(member, topicMsg);
    }
  }

  log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND,
      user->getNickname() + " changed topic of " + channel + " to: " + newTopic);
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

void CommandRouter::handleQuit(User* user, const Command& cmd) {
  std::string reason = cmd.params.empty() ? "Client quit" : cmd.params[0];

  log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND,
      "QUIT command received from: " + user->getNickname() + " (" + reason +
          ")");

  // Broadcast QUIT to all channels the user is in
  // Create copy to avoid iterator invalidation when removing user from channels
  std::set<std::string> channelsCopy = user->getJoinedChannels();
  for (std::set<std::string>::const_iterator it = channelsCopy.begin();
       it != channelsCopy.end(); ++it) {
    Channel* channel = channelManager_->getChannel(*it);
    if (!channel) continue;

    // Send QUIT message to all channel members except the quitting user
    std::string quitMsg = ResponseFormatter::rplQuit(user, reason);
    const std::set<int>& members = channel->getMembers();
    for (std::set<int>::const_iterator mit = members.begin();
         mit != members.end(); ++mit) {
      if (*mit != user->getSocketFd()) {
        User* member = userManager_->getUserByFd(*mit);
        if (member) {
          sendResponse(member, quitMsg);
        }
      }
    }

    // Remove user from channel
    channel->removeMember(user->getSocketFd());
    channel->removeOperator(user->getSocketFd());

    // Remove channel if empty
    if (channel->getMemberCount() == 0) {
      channelManager_->removeChannel(*it);
      log(LOG_LEVEL_INFO, LOG_CATEGORY_CHANNEL,
          "Channel removed: " + *it + " (empty after QUIT)");
    }
  }

  // Note: Actual disconnection is handled by Server layer
  // This just broadcasts the QUIT message to relevant users
}

// ==========================================
// Helpers
// ==========================================

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void CommandRouter::sendResponse(User* user, const std::string& response) {
  if (!user) return;
  user->getWriteBuffer() += response;
}

void CommandRouter::completeRegistration(User* user) {
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
  // RFC1459 strict compliance for nickname validation
  // LIMITATIONS:
  // 1. Does not allow underscore '_' (commonly allowed in modern servers)
  // 2. 9-character limit (modern servers often support longer nicknames)
  // 3. No reserved nickname checking (e.g., "anonymous", server names)
  // These limitations are acceptable for educational/RFC1459 strict compliance

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
