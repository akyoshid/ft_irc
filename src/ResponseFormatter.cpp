#include "ResponseFormatter.hpp"

#include <sstream>

// ==========================================
// Helper methods
// ==========================================

std::string ResponseFormatter::formatMessage(
    const std::string& prefix, const std::string& command,
    const std::vector<std::string>& params) {
  std::string message;

  // Add prefix if provided
  if (!prefix.empty()) {
    message += ":" + prefix + " ";
  }

  // Add command
  message += command;

  // Add parameters
  for (size_t i = 0; i < params.size(); ++i) {
    message += " ";
    // Last parameter with spaces should be prefixed with ':'
    if (i == params.size() - 1 && params[i].find(' ') != std::string::npos) {
      message += ":" + params[i];
    } else {
      message += params[i];
    }
  }

  // Add CRLF termination
  message += "\r\n";
  return message;
}

std::string ResponseFormatter::formatUserPrefix(const User* user) {
  if (!user) return "";

  std::string prefix = user->getNickname();
  if (prefix.empty()) {
    prefix = "*";  // Use * for unregistered users
  }

  prefix += "!" + user->getUsername();
  prefix += "@" + user->getIp();
  return prefix;
}

// ==========================================
// Welcome messages (001-005)
// ==========================================

std::string ResponseFormatter::rplWelcome(const User* user) {
  std::vector<std::string> params;
  params.push_back(user->getNickname());
  params.push_back("Welcome to the ft_irc Network " + formatUserPrefix(user));
  return formatMessage("ft_irc", "001", params);
}

std::string ResponseFormatter::rplYourHost(const User* user) {
  std::vector<std::string> params;
  params.push_back(user->getNickname());
  params.push_back("Your host is ft_irc, running version 1.0");
  return formatMessage("ft_irc", "002", params);
}

std::string ResponseFormatter::rplCreated(const User* user) {
  std::vector<std::string> params;
  params.push_back(user->getNickname());
  params.push_back("This server was created 2025");
  return formatMessage("ft_irc", "003", params);
}

std::string ResponseFormatter::rplMyInfo(const User* user) {
  std::vector<std::string> params;
  params.push_back(user->getNickname());
  params.push_back("ft_irc");
  params.push_back("1.0");
  params.push_back("io");     // User modes
  params.push_back("itkol");  // Channel modes
  return formatMessage("ft_irc", "004", params);
}

// ==========================================
// Command responses
// ==========================================

std::string ResponseFormatter::rplJoin(const User* user,
                                       const std::string& channel) {
  std::vector<std::string> params;
  params.push_back(channel);
  return formatMessage(formatUserPrefix(user), "JOIN", params);
}

std::string ResponseFormatter::rplPart(const User* user,
                                       const std::string& channel,
                                       const std::string& reason) {
  std::vector<std::string> params;
  params.push_back(channel);
  if (!reason.empty()) {
    params.push_back(reason);
  }
  return formatMessage(formatUserPrefix(user), "PART", params);
}

std::string ResponseFormatter::rplPrivmsg(const User* from,
                                          const std::string& target,
                                          const std::string& message) {
  std::vector<std::string> params;
  params.push_back(target);
  params.push_back(message);
  return formatMessage(formatUserPrefix(from), "PRIVMSG", params);
}

std::string ResponseFormatter::rplNotice(const User* from,
                                         const std::string& target,
                                         const std::string& message) {
  std::vector<std::string> params;
  params.push_back(target);
  params.push_back(message);
  return formatMessage(formatUserPrefix(from), "NOTICE", params);
}

std::string ResponseFormatter::rplTopic(const std::string& channel,
                                        const std::string& topic) {
  std::vector<std::string> params;
  params.push_back(channel);
  params.push_back(topic);
  return formatMessage("ft_irc", "332", params);
}

std::string ResponseFormatter::rplKick(const User* kicker,
                                       const std::string& channel,
                                       const std::string& kicked,
                                       const std::string& reason) {
  std::vector<std::string> params;
  params.push_back(channel);
  params.push_back(kicked);
  if (!reason.empty()) {
    params.push_back(reason);
  }
  return formatMessage(formatUserPrefix(kicker), "KICK", params);
}

// ==========================================
// Error responses (400-599)
// ==========================================

std::string ResponseFormatter::errNoSuchNick(const std::string& nickname) {
  std::vector<std::string> params;
  params.push_back(nickname);
  params.push_back("No such nick/channel");
  return formatMessage("ft_irc", "401", params);
}

std::string ResponseFormatter::errNoSuchChannel(const std::string& channel) {
  std::vector<std::string> params;
  params.push_back(channel);
  params.push_back("No such channel");
  return formatMessage("ft_irc", "403", params);
}

std::string ResponseFormatter::errCannotSendToChan(const std::string& channel) {
  std::vector<std::string> params;
  params.push_back(channel);
  params.push_back("Cannot send to channel");
  return formatMessage("ft_irc", "404", params);
}

std::string ResponseFormatter::errTooManyChannels(const std::string& channel) {
  std::vector<std::string> params;
  params.push_back(channel);
  params.push_back("You have joined too many channels");
  return formatMessage("ft_irc", "405", params);
}

std::string ResponseFormatter::errNicknameInUse(const std::string& nickname) {
  std::vector<std::string> params;
  params.push_back(nickname);
  params.push_back("Nickname is already in use");
  return formatMessage("ft_irc", "433", params);
}

std::string ResponseFormatter::errNotOnChannel(const std::string& channel) {
  std::vector<std::string> params;
  params.push_back(channel);
  params.push_back("You're not on that channel");
  return formatMessage("ft_irc", "442", params);
}

std::string ResponseFormatter::errUserOnChannel(const std::string& user,
                                                const std::string& channel) {
  std::vector<std::string> params;
  params.push_back(user);
  params.push_back(channel);
  params.push_back("is already on channel");
  return formatMessage("ft_irc", "443", params);
}

std::string ResponseFormatter::errNeedMoreParams(const std::string& command) {
  std::vector<std::string> params;
  params.push_back(command);
  params.push_back("Not enough parameters");
  return formatMessage("ft_irc", "461", params);
}

std::string ResponseFormatter::errAlreadyRegistered() {
  std::vector<std::string> params;
  params.push_back("You may not reregister");
  return formatMessage("ft_irc", "462", params);
}

std::string ResponseFormatter::errPasswdMismatch() {
  std::vector<std::string> params;
  params.push_back("Password incorrect");
  return formatMessage("ft_irc", "464", params);
}

std::string ResponseFormatter::errChannelIsFull(const std::string& channel) {
  std::vector<std::string> params;
  params.push_back(channel);
  params.push_back("Cannot join channel (+l)");
  return formatMessage("ft_irc", "471", params);
}

std::string ResponseFormatter::errInviteOnlyChan(const std::string& channel) {
  std::vector<std::string> params;
  params.push_back(channel);
  params.push_back("Cannot join channel (+i)");
  return formatMessage("ft_irc", "473", params);
}

std::string ResponseFormatter::errBadChannelKey(const std::string& channel) {
  std::vector<std::string> params;
  params.push_back(channel);
  params.push_back("Cannot join channel (+k)");
  return formatMessage("ft_irc", "475", params);
}

std::string ResponseFormatter::errChanOPrivsNeeded(const std::string& channel) {
  std::vector<std::string> params;
  params.push_back(channel);
  params.push_back("You're not channel operator");
  return formatMessage("ft_irc", "482", params);
}
