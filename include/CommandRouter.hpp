#ifndef INCLUDE_COMMANDROUTER_HPP_
#define INCLUDE_COMMANDROUTER_HPP_

#include <string>
#include <vector>

#include "ChannelManager.hpp"
#include "ResponseFormatter.hpp"
#include "User.hpp"
#include "UserManager.hpp"

// Command: Represents a parsed IRC command
struct Command {
  std::string prefix;   // Optional prefix (usually empty from clients)
  std::string command;  // Command name (PASS, NICK, JOIN, etc.)
  std::vector<std::string> params;  // Command parameters

  Command() : prefix(""), command("") {}
};

// CommandRouter: Parses and routes IRC commands to appropriate handlers
// Implements IRC message parsing per RFC1459 and dispatches to command handlers
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
  // NOTE: Password stored in plain text for educational purposes
  // Production systems should use secure memory handling (e.g., mlock,
  // explicit zeroing) C++98 has limited options for secure string handling
  std::string password_;

  // ==========================================
  // Parser
  // ==========================================
  Command parseCommand(const std::string& message);

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

  // MODE command helpers
  static std::string getCurrentModes(Channel* chan);
  static void applyModeInviteOnly(Channel* chan, bool adding,
                                  std::string& appliedModes);
  static void applyModeTopicRestricted(Channel* chan, bool adding,
                                       std::string& appliedModes);
  void applyModeKey(User* sender, Channel* chan, bool adding, size_t& argIndex,
                    const std::vector<std::string>& params,
                    std::string& appliedModes, std::string& appliedArgs);
  void applyModeOperator(User* sender, Channel* chan, bool adding,
                         size_t& argIndex,
                         const std::vector<std::string>& params,
                         std::string& appliedModes, std::string& appliedArgs);
  void applyModeUserLimit(User* sender, Channel* chan, bool adding,
                          size_t& argIndex,
                          const std::vector<std::string>& params,
                          std::string& appliedModes, std::string& appliedArgs);
  void broadcastModeChange(User* user, const std::string& channel,
                           const std::string& appliedModes,
                           const std::string& appliedArgs, Channel* chan);

  CommandRouter();                                     // = delete
  CommandRouter(const CommandRouter& src);             // = delete
  CommandRouter& operator=(const CommandRouter& src);  // = delete
};

#endif
