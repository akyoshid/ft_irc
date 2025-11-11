#include "CommandRouter.hpp"

#include <set>
#include <string>
#include <vector>

#include "utils.hpp"

CommandRouter::CommandRouter(UserManager* userMgr, ChannelManager* chanMgr,
                             EventLoop* eventLoop, const std::string& password)
    : userManager_(userMgr),
      channelManager_(chanMgr),
      eventLoop_(eventLoop),
      parser_(new CommandParser()),
      password_(password) {}

CommandRouter::~CommandRouter() { delete parser_; }

// ==========================================
// Main entry point
// ==========================================

CommandResult CommandRouter::processMessage(User* user,
                                            const std::string& message) {
  if (!user) {
    log(LOG_LEVEL_WARNING, LOG_CATEGORY_COMMAND,
        "processMessage called with NULL user");
    return CMD_CONTINUE;
  }

  log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND, user->getIp() + ": " + message);

  try {
    Command cmd = parser_->parseCommand(message);
    return dispatch(user, cmd);
  } catch (const std::exception& e) {
    // Log detailed error internally
    log(LOG_LEVEL_WARNING, LOG_CATEGORY_COMMAND,
        "Failed to parse command from " + user->getIp() + ": " + e.what());
    // Send sanitized error response to client (don't expose internal details)
    sendResponse(user, "ERROR :Invalid message format\r\n");
    return CMD_CONTINUE;
  }
}

// ==========================================
// Dispatcher
// ==========================================

CommandResult CommandRouter::dispatch(User* user, const Command& cmd) {
  if (cmd.command == "CAP") {
    handleCap(user, cmd);
  } else if (cmd.command == "PASS") {
    handlePass(user, cmd);
  } else if (cmd.command == "NICK") {
    handleNick(user, cmd);
  } else if (cmd.command == "USER") {
    handleUser(user, cmd);
  } else if (cmd.command == "PING") {
    handlePing(user, cmd);
  } else if (cmd.command == "PONG") {
    handlePong(user, cmd);
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
    return CMD_DISCONNECT;
  } else {
    log(LOG_LEVEL_WARNING, LOG_CATEGORY_COMMAND,
        "Unknown command: " + cmd.command);
    sendResponse(user, ResponseFormatter::errUnknownCommand(user->getNickname(), cmd.command));
  }
  return CMD_CONTINUE;
}

// ==========================================
// Command handlers (stub implementations)
// ==========================================

void CommandRouter::handlePass(User* user, const Command& cmd) {
  log(LOG_LEVEL_DEBUG, LOG_CATEGORY_COMMAND,
      "PASS command from " + user->getIp() + " (password hidden for security)");
  // Check if user is already registered
  if (user->isRegistered()) {
    sendResponse(user, ResponseFormatter::errAlreadyRegistered(user->getNickname().empty() ? "*" : user->getNickname()));
    return;
  }

  // Check parameter count
  if (cmd.params.empty()) {
    sendResponse(user, ResponseFormatter::errNeedMoreParams(user->getNickname().empty() ? "*" : user->getNickname(), "PASS"));
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
    sendResponse(user, ResponseFormatter::errPasswdMismatch(user->getNickname().empty() ? "*" : user->getNickname()));
    return;
  }

  // Set authenticated flag
  user->setAuthenticated(true);
  log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND,
      "Authentication successful for " + user->getIp());
}

void CommandRouter::handleNick(User* user, const Command& cmd) {
  std::string params = cmd.params.empty() ? "(no params)" : cmd.params[0];
  log(LOG_LEVEL_DEBUG, LOG_CATEGORY_COMMAND,
      "NICK command from " + user->getIp() + " params: [" + params + "]");
  // Check parameter count
  if (cmd.params.empty()) {
    sendResponse(user, ResponseFormatter::errNeedMoreParams(user->getNickname(), "NICK"));
    return;
  }

  const std::string& newNick = cmd.params[0];

  // Validate nickname format
  if (!isValidNickname(newNick)) {
    sendResponse(user, ResponseFormatter::errErroneusNickname(user->getNickname(), newNick));
    return;
  }

  // Check if nickname is already in use by another user
  // NOTE: Nickname comparison is case-insensitive per RFC1459
  // UserManager normalizes nicknames internally
  // NOTE: This check-then-set pattern is safe in single-threaded context
  // Multi-threaded implementation would require atomic check-and-set
  if (userManager_->isNicknameInUse(newNick)) {
    sendResponse(user, ResponseFormatter::errNicknameInUse(user->getNickname(), newNick));
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
  std::string params;
  for (size_t i = 0; i < cmd.params.size(); ++i) {
    if (i > 0) params += ", ";
    params += cmd.params[i];
  }
  log(LOG_LEVEL_DEBUG, LOG_CATEGORY_COMMAND,
      "USER command from " + user->getIp() + " params: [" + params + "]");
  // Check if user is already registered
  if (user->isRegistered()) {
    sendResponse(user, ResponseFormatter::errAlreadyRegistered(user->getNickname().empty() ? "*" : user->getNickname()));
    return;
  }

  // Check parameter count (username, hostname, servername, realname)
  if (cmd.params.size() < 4) {
    sendResponse(user, ResponseFormatter::errNeedMoreParams(user->getNickname().empty() ? "*" : user->getNickname(), "USER"));
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
  std::string params;
  for (size_t i = 0; i < cmd.params.size(); ++i) {
    if (i > 0) params += ", ";
    params += cmd.params[i];
  }
  log(LOG_LEVEL_DEBUG, LOG_CATEGORY_COMMAND,
      "JOIN command from " + user->getNickname() + " params: [" + params + "]");
  // Check if user is registered
  if (!user->isRegistered()) {
    return;  // Silently ignore commands from unregistered users
  }

  // Check parameter count
  if (cmd.params.empty()) {
    sendResponse(user, ResponseFormatter::errNeedMoreParams(user->getNickname(), "JOIN"));
    return;
  }

  const std::string& channelName = cmd.params[0];

  // Validate channel name
  // NOTE: Channel names are case-insensitive per RFC1459
  // ChannelManager normalizes names internally
  if (!isValidChannelName(channelName)) {
    sendResponse(user, ResponseFormatter::errNoSuchChannel(user->getNickname(), channelName));
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
    sendResponse(user, ResponseFormatter::errInviteOnlyChan(user->getNickname(), channelName));
    return;
  }

  if (channel->hasUserLimit() &&
      channel->getMemberCount() >= channel->getUserLimit()) {
    sendResponse(user, ResponseFormatter::errChannelIsFull(user->getNickname(), channelName));
    return;
  }

  // Check channel key if set
  if (!channel->getKey().empty()) {
    std::string providedKey = cmd.params.size() > 1 ? cmd.params[1] : "";
    if (providedKey != channel->getKey()) {
      sendResponse(user, ResponseFormatter::errBadChannelKey(user->getNickname(), channelName));
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
  log(LOG_LEVEL_DEBUG, LOG_CATEGORY_CHANNEL,
      "Broadcasting JOIN to " + channelName);
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
  std::string params;
  for (size_t i = 0; i < cmd.params.size(); ++i) {
    if (i > 0) params += ", ";
    params += cmd.params[i];
  }
  log(LOG_LEVEL_DEBUG, LOG_CATEGORY_COMMAND,
      "PART command from " + user->getNickname() + " params: [" + params + "]");
  // Check if user is registered
  if (!user->isRegistered()) {
    return;  // Silently ignore commands from unregistered users
  }

  // Check parameter count
  if (cmd.params.empty()) {
    sendResponse(user, ResponseFormatter::errNeedMoreParams(user->getNickname(), "PART"));
    return;
  }

  const std::string& channelName = cmd.params[0];
  std::string reason = cmd.params.size() > 1 ? cmd.params[1] : "";

  // Get channel
  Channel* channel = channelManager_->getChannel(channelName);
  if (!channel) {
    sendResponse(user, ResponseFormatter::errNoSuchChannel(user->getNickname(), channelName));
    return;
  }

  // Check if user is in channel
  if (!channel->isMember(user->getSocketFd())) {
    sendResponse(user, ResponseFormatter::errNotOnChannel(user->getNickname(), channelName));
    return;
  }

  // Broadcast PART to all channel members (including the user)
  log(LOG_LEVEL_DEBUG, LOG_CATEGORY_CHANNEL,
      "Broadcasting PART from " + channelName);
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
  std::string params;
  for (size_t i = 0; i < cmd.params.size(); ++i) {
    if (i > 0) params += ", ";
    params += cmd.params[i];
  }
  log(LOG_LEVEL_DEBUG, LOG_CATEGORY_COMMAND,
      "PRIVMSG command from " + user->getNickname() + " params: [" + params +
          "]");
  // Check if user is registered
  if (!user->isRegistered()) {
    return;  // Silently ignore commands from unregistered users
  }

  // Check parameter count
  if (cmd.params.size() < 2) {
    sendResponse(user, ResponseFormatter::errNeedMoreParams(user->getNickname(), "PRIVMSG"));
    return;
  }

  const std::string& target = cmd.params[0];
  const std::string& message = cmd.params[1];

  // Check if target is a channel or user
  if (!target.empty() && (target[0] == '#' || target[0] == '&')) {
    // Channel message
    Channel* channel = channelManager_->getChannel(target);
    if (!channel) {
      sendResponse(user, ResponseFormatter::errNoSuchChannel(user->getNickname(), target));
      return;
    }

    // Check if user is in channel
    if (!channel->isMember(user->getSocketFd())) {
      sendResponse(user, ResponseFormatter::errCannotSendToChan(user->getNickname(), target));
      return;
    }

    // TODO(Phase 5): Check moderated mode (+m) - only ops/voiced users can send
    // TODO(Phase 5): Check no-external messages (+n) - handled by membership
    // check above

    // log(LOG_LEVEL_DEBUG, LOG_CATEGORY_COMMAND,
    //     "Broadcasting PRIVMSG to " + target);
    log(LOG_LEVEL_DEBUG, LOG_CATEGORY_COMMAND,
        "Queueing PRIVMSG to " + target + " members");
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
      sendResponse(user, ResponseFormatter::errNoSuchNick(user->getNickname(), target));
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
  std::string params;
  for (size_t i = 0; i < cmd.params.size(); ++i) {
    if (i > 0) params += ", ";
    params += cmd.params[i];
  }
  log(LOG_LEVEL_DEBUG, LOG_CATEGORY_COMMAND,
      "KICK command from " + user->getNickname() + " params: [" + params + "]");
  // Check if user is registered
  if (!user->isRegistered()) {
    return;  // Silently ignore commands from unregistered users
  }

  // KICK <channel> <user> [:<reason>]
  if (cmd.params.size() < 2) {
    sendResponse(user, ResponseFormatter::errNeedMoreParams(user->getNickname(), "KICK"));
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
    sendResponse(user, ResponseFormatter::errNoSuchChannel(user->getNickname(), channel));
    return;
  }

  // Check if kicker is on the channel
  if (!chan->isMember(user->getSocketFd())) {
    sendResponse(user, ResponseFormatter::errNotOnChannel(user->getNickname(), channel));
    return;
  }

  // Check if kicker is an operator
  if (!chan->isOperator(user->getSocketFd())) {
    sendResponse(user, ResponseFormatter::errChanOPrivsNeeded(user->getNickname(), channel));
    return;
  }

  // Check if target user exists
  User* targetUser = userManager_->getUserByNickname(targetNick);
  if (!targetUser) {
    sendResponse(user, ResponseFormatter::errNoSuchNick(user->getNickname(), targetNick));
    return;
  }

  // Check if target is on the channel
  if (!chan->isMember(targetUser->getSocketFd())) {
    sendResponse(user,
                 ResponseFormatter::errUserNotInChannel(user->getNickname(), targetNick, channel));
    return;
  }

  // Broadcast KICK message to all channel members
  log(LOG_LEVEL_DEBUG, LOG_CATEGORY_COMMAND, "Broadcasting KICK to " + channel);
  std::string kickMsg =
      ResponseFormatter::rplKick(user, channel, targetNick, reason);
  const std::set<int>& members = chan->getMembers();
  for (std::set<int>::const_iterator it = members.begin(); it != members.end();
       ++it) {
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
  std::string params;
  for (size_t i = 0; i < cmd.params.size(); ++i) {
    if (i > 0) params += ", ";
    params += cmd.params[i];
  }
  log(LOG_LEVEL_DEBUG, LOG_CATEGORY_COMMAND,
      "INVITE command from " + user->getNickname() + " params: [" + params +
          "]");
  // Check if user is registered
  if (!user->isRegistered()) {
    return;  // Silently ignore commands from unregistered users
  }

  // INVITE <nickname> <channel>
  if (cmd.params.size() < 2) {
    sendResponse(user, ResponseFormatter::errNeedMoreParams(user->getNickname(), "INVITE"));
    return;
  }

  const std::string& targetNick = cmd.params[0];
  const std::string& channel = cmd.params[1];

  // Check if target user exists
  User* targetUser = userManager_->getUserByNickname(targetNick);
  if (!targetUser) {
    sendResponse(user, ResponseFormatter::errNoSuchNick(user->getNickname(), targetNick));
    return;
  }

  // Check if channel exists
  Channel* chan = channelManager_->getChannel(channel);
  if (!chan) {
    sendResponse(user, ResponseFormatter::errNoSuchChannel(user->getNickname(), channel));
    return;
  }

  // Check if inviter is on the channel
  if (!chan->isMember(user->getSocketFd())) {
    sendResponse(user, ResponseFormatter::errNotOnChannel(user->getNickname(), channel));
    return;
  }

  // Check if target is already on the channel
  if (chan->isMember(targetUser->getSocketFd())) {
    sendResponse(user,
                 ResponseFormatter::errUserOnChannel(user->getNickname(), targetNick, channel));
    return;
  }

  // If channel is invite-only, only operators can invite
  if (chan->isInviteOnly() && !chan->isOperator(user->getSocketFd())) {
    sendResponse(user, ResponseFormatter::errChanOPrivsNeeded(user->getNickname(), channel));
    return;
  }

  // Add target to invite list
  chan->addInvite(targetUser->getSocketFd());

  // Send confirmation to inviter (341 RPL_INVITING)
  sendResponse(user, ResponseFormatter::rplInviting(user->getNickname(),
                                                    targetNick, channel));

  // Send INVITE message to target
  sendResponse(targetUser,
               ResponseFormatter::rplInvite(user, targetNick, channel));

  log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND,
      user->getNickname() + " invited " + targetNick + " to " + channel);
}

void CommandRouter::handleTopic(User* user, const Command& cmd) {
  std::string params;
  for (size_t i = 0; i < cmd.params.size(); ++i) {
    if (i > 0) params += ", ";
    params += cmd.params[i];
  }
  log(LOG_LEVEL_DEBUG, LOG_CATEGORY_COMMAND,
      "TOPIC command from " + user->getNickname() + " params: [" + params +
          "]");
  // Check if user is registered
  if (!user->isRegistered()) {
    return;  // Silently ignore commands from unregistered users
  }

  // TOPIC <channel> [:<topic>]
  if (cmd.params.empty()) {
    sendResponse(user, ResponseFormatter::errNeedMoreParams(user->getNickname(), "TOPIC"));
    return;
  }

  const std::string& channel = cmd.params[0];

  // Check if channel exists
  Channel* chan = channelManager_->getChannel(channel);
  if (!chan) {
    sendResponse(user, ResponseFormatter::errNoSuchChannel(user->getNickname(), channel));
    return;
  }

  // Check if user is on the channel
  if (!chan->isMember(user->getSocketFd())) {
    sendResponse(user, ResponseFormatter::errNotOnChannel(user->getNickname(), channel));
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
    sendResponse(user, ResponseFormatter::errChanOPrivsNeeded(user->getNickname(), channel));
    return;
  }

  // Set new topic
  const std::string& newTopic = cmd.params[1];
  chan->setTopic(newTopic);

  // Broadcast topic change to all channel members
  log(LOG_LEVEL_DEBUG, LOG_CATEGORY_COMMAND,
      "Broadcasting TOPIC to " + channel);
  std::string topicMsg =
      ResponseFormatter::rplTopicChange(user, channel, newTopic);
  const std::set<int>& members = chan->getMembers();
  for (std::set<int>::const_iterator it = members.begin(); it != members.end();
       ++it) {
    User* member = userManager_->getUserByFd(*it);
    if (member) {
      sendResponse(member, topicMsg);
    }
  }

  log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND,
      user->getNickname() + " changed topic of " + channel +
          " to: " + newTopic);
}

void CommandRouter::handleMode(User* user, const Command& cmd) {
  std::string params;
  for (size_t i = 0; i < cmd.params.size(); ++i) {
    if (i > 0) params += ", ";
    params += cmd.params[i];
  }
  log(LOG_LEVEL_DEBUG, LOG_CATEGORY_COMMAND,
      "MODE command from " + user->getNickname() + " params: [" + params + "]");
  // Check if user is registered
  if (!user->isRegistered()) {
    return;  // Silently ignore commands from unregistered users
  }

  // MODE <channel> [<modestring> [<mode arguments>...]]
  if (cmd.params.empty()) {
    sendResponse(user, ResponseFormatter::errNeedMoreParams(user->getNickname(), "MODE"));
    return;
  }

  const std::string& channel = cmd.params[0];

  // Check if channel exists
  Channel* chan = channelManager_->getChannel(channel);
  if (!chan) {
    sendResponse(user, ResponseFormatter::errNoSuchChannel(user->getNickname(), channel));
    return;
  }

  // Check if user is on the channel
  if (!chan->isMember(user->getSocketFd())) {
    sendResponse(user, ResponseFormatter::errNotOnChannel(user->getNickname(), channel));
    return;
  }

  // If no mode string, return current modes
  if (cmd.params.size() == 1) {
    sendResponse(user, ResponseFormatter::rplChannelModeIs(
                           channel, getCurrentModes(chan)));
    return;
  }

  // Check if user is operator for mode changes
  if (!chan->isOperator(user->getSocketFd())) {
    sendResponse(user, ResponseFormatter::errChanOPrivsNeeded(user->getNickname(), channel));
    return;
  }

  // Parse mode string
  const std::string& modeString = cmd.params[1];

  // Validate mode string is not empty
  if (modeString.empty()) {
    sendResponse(user, ResponseFormatter::errNeedMoreParams(user->getNickname(), "MODE"));
    return;
  }

  bool adding = true;
  size_t argIndex = 2;
  std::string appliedModes;
  std::string appliedArgs;

  for (size_t i = 0; i < modeString.length(); ++i) {
    char mode = modeString[i];

    if (mode == '+') {
      adding = true;
      continue;
    }
    if (mode == '-') {
      adding = false;
      continue;
    }

    // Handle each mode
    if (mode == 'i') {
      applyModeInviteOnly(chan, adding, appliedModes);
    } else if (mode == 't') {
      applyModeTopicRestricted(chan, adding, appliedModes);
    } else if (mode == 'k') {
      applyModeKey(user, chan, adding, argIndex, cmd.params, appliedModes,
                   appliedArgs);
    } else if (mode == 'o') {
      applyModeOperator(user, chan, adding, argIndex, cmd.params, appliedModes,
                        appliedArgs);
    } else if (mode == 'l') {
      applyModeUserLimit(user, chan, adding, argIndex, cmd.params, appliedModes,
                         appliedArgs);
    } else {
      sendResponse(user, ResponseFormatter::errUnknownMode(user->getNickname(), mode));
    }
  }

  // Broadcast mode change to all channel members if any modes were applied
  if (!appliedModes.empty()) {
    broadcastModeChange(user, channel, appliedModes, appliedArgs, chan);
  }
}

void CommandRouter::handleQuit(User* user, const Command& cmd) {
  std::string params;
  for (size_t i = 0; i < cmd.params.size(); ++i) {
    if (i > 0) params += ", ";
    params += cmd.params[i];
  }
  log(LOG_LEVEL_DEBUG, LOG_CATEGORY_COMMAND,
      "QUIT command from " + user->getNickname() + " params: [" + params + "]");
  std::string reason = cmd.params.empty() ? "Client quit" : cmd.params[0];

  log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND,
      "QUIT command received from: " + user->getNickname() + " (" + reason +
          ")");

  // Send QUIT confirmation to the user
  std::string quitMsg = ResponseFormatter::rplQuit(user, reason);
  sendResponse(user, quitMsg);

  // Broadcast QUIT to all channels the user is in
  log(LOG_LEVEL_DEBUG, LOG_CATEGORY_COMMAND,
      "Broadcasting QUIT from " + user->getNickname());
  // Create copy to avoid iterator invalidation when removing user from channels
  std::set<std::string> channelsCopy = user->getJoinedChannels();
  for (std::set<std::string>::const_iterator it = channelsCopy.begin();
       it != channelsCopy.end(); ++it) {
    Channel* channel = channelManager_->getChannel(*it);
    if (!channel) continue;

    // Send QUIT message to all channel members except the quitting user
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
    user->leaveChannel(*it);
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

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void CommandRouter::handleCap(User* user, const Command& cmd) {
  std::string params;
  for (size_t i = 0; i < cmd.params.size(); ++i) {
    if (i > 0) params += ", ";
    params += cmd.params[i];
  }
  log(LOG_LEVEL_DEBUG, LOG_CATEGORY_COMMAND,
      "CAP command from " + user->getIp() + " params: [" + params + "]");
  // CAP command is sent by modern IRC clients for capability negotiation
  // We don't support any capabilities, so just silently ignore it
  // This prevents "Unknown command" errors for clients using CAP
  (void)user;  // Suppress unused parameter warning
  (void)cmd;   // Suppress unused parameter warning
}

void CommandRouter::handlePing(User* user, const Command& cmd) {
  // PING: Keep-alive mechanism
  // Reply with PONG using the same token

  // Debug: Log the full PING command received
  std::string debugMsg =
      "PING command - prefix: [" + cmd.prefix + "], params: [";
  for (size_t i = 0; i < cmd.params.size(); ++i) {
    if (i > 0) debugMsg += ", ";
    debugMsg += cmd.params[i];
  }
  debugMsg += "]";
  log(LOG_LEVEL_DEBUG, LOG_CATEGORY_COMMAND,
      "PING from " + user->getIp() + " - " + debugMsg);

  // RFC 1459/2812: Server responses must have prefix ":server"
  // Format: ":server PONG server <token>" or ":server PONG server :<token>"
  // Using trailing parameter (:token) for safety with multi-word tokens
  std::string response;
  if (cmd.params.empty()) {
    // No token provided
    response = ":ft_irc PONG ft_irc\r\n";
  } else {
    // Echo back the token from PING
    std::string token = cmd.params[0];
    response = ":ft_irc PONG ft_irc :" + token + "\r\n";
  }

  log(LOG_LEVEL_DEBUG, LOG_CATEGORY_COMMAND,
      "PONG response to " + user->getIp() + ": [" +
          response.substr(0, response.length() - 2) + "]");
  sendResponse(user, response);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void CommandRouter::handlePong(User* user, const Command& cmd) {
  // PONG: Response to server's PING
  // Currently we don't send PING to clients, but acknowledge PONG if received

  log(LOG_LEVEL_DEBUG, LOG_CATEGORY_COMMAND,
      "PONG received from: " + user->getNickname());

  (void)cmd;  // Suppress unused parameter warning
  // No response needed for PONG
}

// ==========================================
// MODE command helper implementations
// ==========================================

std::string CommandRouter::getCurrentModes(Channel* chan) {
  std::string modes = "+";
  if (chan->isInviteOnly()) modes += "i";
  if (chan->isTopicRestricted()) modes += "t";
  if (chan->hasUserLimit()) modes += "l";
  if (!chan->getKey().empty()) modes += "k";
  return modes;
}

void CommandRouter::applyModeInviteOnly(Channel* chan, bool adding,
                                        std::string& appliedModes) {
  chan->setInviteOnly(adding);
  appliedModes += adding ? "+i" : "-i";
}

void CommandRouter::applyModeTopicRestricted(Channel* chan, bool adding,
                                             std::string& appliedModes) {
  chan->setTopicRestricted(adding);
  appliedModes += adding ? "+t" : "-t";
}

void CommandRouter::applyModeKey(User* sender, Channel* chan, bool adding,
                                 size_t& argIndex,
                                 const std::vector<std::string>& params,
                                 std::string& appliedModes,
                                 std::string& appliedArgs) {
  if (adding) {
    if (argIndex < params.size()) {
      const std::string& key = params[argIndex];

      // Validate key according to RFC 1459
      // Keys should not contain spaces, commas, or control characters
      if (key.empty()) {
        sendResponse(sender, ResponseFormatter::errInvalidModeParam(
                                 sender->getNickname(), chan->getName(), 'k', key,
                                 "Invalid key: empty parameter"));
        ++argIndex;
        return;
      }

      // Check for invalid characters
      for (size_t i = 0; i < key.length(); ++i) {
        char c = key[i];
        // Disallow spaces, commas, and control characters (< 0x20 or == 0x7F)
        if (c == ' ' || c == ',' || c < 0x20 || c == 0x7F) {
          sendResponse(sender, ResponseFormatter::errInvalidModeParam(
                                   sender->getNickname(), chan->getName(), 'k', key,
                                   "Invalid key: contains invalid characters"));
          ++argIndex;
          return;
        }
      }

      // Enforce reasonable length limit (23 chars per RFC 1459)
      if (key.length() > 23) {
        sendResponse(sender, ResponseFormatter::errInvalidModeParam(
                                 sender->getNickname(), chan->getName(), 'k', key,
                                 "Invalid key: too long (max 23 characters)"));
        ++argIndex;
        return;
      }

      chan->setKey(key);
      appliedModes += "+k";
      appliedArgs += appliedArgs.empty() ? "" : " ";
      appliedArgs += key;
      ++argIndex;
    }
  } else {
    chan->clearKey();
    appliedModes += "-k";
  }
}

void CommandRouter::applyModeOperator(User* sender, Channel* chan, bool adding,
                                      size_t& argIndex,
                                      const std::vector<std::string>& params,
                                      std::string& appliedModes,
                                      std::string& appliedArgs) {
  if (argIndex >= params.size()) return;

  const std::string& targetNick = params[argIndex];
  User* targetUser = userManager_->getUserByNickname(targetNick);

  if (!targetUser) {
    sendResponse(sender, ResponseFormatter::errNoSuchNick(sender->getNickname(), targetNick));
    ++argIndex;
    return;
  }

  if (!chan->isMember(targetUser->getSocketFd())) {
    sendResponse(sender, ResponseFormatter::errUserNotInChannel(
                             sender->getNickname(), targetNick, chan->getName()));
    ++argIndex;
    return;
  }

  if (adding) {
    chan->addOperator(targetUser->getSocketFd());
  } else {
    // Prevent last operator from removing their own operator status
    if (targetUser->getSocketFd() == sender->getSocketFd() &&
        chan->getOperators().size() == 1) {
      sendResponse(sender,
                   ResponseFormatter::errChanOPrivsNeeded(sender->getNickname(), chan->getName()));
      ++argIndex;
      return;
    }
    chan->removeOperator(targetUser->getSocketFd());
  }
  appliedModes += adding ? "+o" : "-o";
  appliedArgs += appliedArgs.empty() ? "" : " ";
  appliedArgs += targetNick;
  ++argIndex;
}

void CommandRouter::applyModeUserLimit(User* sender, Channel* chan, bool adding,
                                       size_t& argIndex,
                                       const std::vector<std::string>& params,
                                       std::string& appliedModes,
                                       std::string& appliedArgs) {
  if (adding) {
    if (argIndex < params.size()) {
      const std::string& limitStr = params[argIndex];

      // Validate: non-empty and reasonable length
      if (limitStr.empty()) {
        sendResponse(sender, ResponseFormatter::errInvalidModeParam(
                                 sender->getNickname(), chan->getName(), 'l', limitStr,
                                 "Invalid limit: empty parameter"));
        ++argIndex;
        return;
      }

      if (limitStr.length() > 10) {
        sendResponse(sender, ResponseFormatter::errInvalidModeParam(
                                 sender->getNickname(), chan->getName(), 'l', limitStr,
                                 "Invalid limit: too large"));
        ++argIndex;
        return;
      }

      // Check all characters are digits
      for (size_t j = 0; j < limitStr.length(); ++j) {
        if (limitStr[j] < '0' || limitStr[j] > '9') {
          sendResponse(sender, ResponseFormatter::errInvalidModeParam(
                                   sender->getNickname(), chan->getName(), 'l', limitStr,
                                   "Invalid limit: not a number"));
          ++argIndex;
          return;
        }
      }

      // Parse with overflow protection
      size_t limit = 0;
      const size_t maxLimit =
          static_cast<size_t>(-1);  // SIZE_MAX equivalent for C++98
      for (size_t j = 0; j < limitStr.length(); ++j) {
        size_t digit = limitStr[j] - '0';
        // Check for overflow before multiplication
        if (limit > (maxLimit - digit) / 10) {
          sendResponse(sender, ResponseFormatter::errInvalidModeParam(
                                   sender->getNickname(), chan->getName(), 'l', limitStr,
                                   "Invalid limit: number too large"));
          ++argIndex;
          return;
        }
        limit = (limit * 10) + digit;
      }

      if (limit > 0) {
        chan->setUserLimit(limit);
        appliedModes += "+l";
        appliedArgs += appliedArgs.empty() ? "" : " ";
        appliedArgs += limitStr;
      }
      ++argIndex;
    }
  } else {
    chan->clearUserLimit();
    appliedModes += "-l";
  }
}

void CommandRouter::broadcastModeChange(User* user, const std::string& channel,
                                        const std::string& appliedModes,
                                        const std::string& appliedArgs,
                                        Channel* chan) {
  std::string modeMsg = ResponseFormatter::rplModeChange(
      user, channel, appliedModes, appliedArgs);
  const std::set<int>& members = chan->getMembers();
  for (std::set<int>::const_iterator it = members.begin(); it != members.end();
       ++it) {
    User* member = userManager_->getUserByFd(*it);
    if (member) {
      sendResponse(member, modeMsg);
    }
  }
  log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND,
      user->getNickname() + " set mode " + appliedModes + " on " + channel);
}

// ==========================================
// Helpers
// ==========================================

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void CommandRouter::sendResponse(User* user, const std::string& response) {
  if (!user) return;
  bool wasEmpty = user->getWriteBuffer().empty();
  user->getWriteBuffer() += response;
  // Register EPOLLOUT only when buffer transitions from empty to non-empty
  if (wasEmpty) {
    eventLoop_->modifyFd(user->getSocketFd(), EPOLLIN | EPOLLOUT);
  }
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
