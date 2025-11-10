#ifndef INCLUDE_COMMANDROUTER_HPP_
#define INCLUDE_COMMANDROUTER_HPP_

#include <string>

#include "ChannelManager.hpp"
#include "CommandParser.hpp"
#include "ResponseFormatter.hpp"
#include "User.hpp"
#include "UserManager.hpp"

// CommandRouter: Routes IRC commands to appropriate handlers
// Dispatches parsed commands to command handlers
class CommandRouter {
 public:
  CommandRouter(UserManager* userMgr, ChannelManager* chanMgr,
                const std::string& password);
  ~CommandRouter();

  // Parse and execute IRC command from user
  // message: Raw IRC message from client
  void processMessage(User* user, const std::string& message);

 private:
  UserManager* userManager_;
  ChannelManager* channelManager_;
  CommandParser* parser_;
  // NOTE: Password stored in plain text for educational purposes
  // Production systems should use secure memory handling (e.g., mlock,
  // explicit zeroing) C++98 has limited options for secure string handling
  std::string password_;

  // ==========================================
  // Dispatcher
  // ==========================================
  void dispatch(User* user, const Command& cmd);

  // ==========================================
  // Command handlers (stub implementations for Phase 2)
  // Phase 3+ will implement full logic
  // ==========================================
  void handlePass(User* user, const Command& cmd);
  void handleNick(User* user, const Command& cmd);
  void handleUser(User* user, const Command& cmd);
  void handleJoin(User* user, const Command& cmd);
  void handlePart(User* user, const Command& cmd);
  void handlePrivmsg(User* user, const Command& cmd);
  void handleKick(User* user, const Command& cmd);
  void handleInvite(User* user, const Command& cmd);
  void handleTopic(User* user, const Command& cmd);
  void handleMode(User* user, const Command& cmd);
  void handleQuit(User* user, const Command& cmd);

  // ==========================================
  // Helpers
  // ==========================================
  void sendResponse(User* user, const std::string& response);
  void completeRegistration(User* user);
  bool isValidChannelName(const std::string& name);
  bool isValidNickname(const std::string& nickname);

  CommandRouter();                                     // = delete
  CommandRouter(const CommandRouter& src);             // = delete
  CommandRouter& operator=(const CommandRouter& src);  // = delete
};

#endif
