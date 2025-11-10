#ifndef INCLUDE_RESPONSEFORMATTER_HPP_
#define INCLUDE_RESPONSEFORMATTER_HPP_

#include <string>
#include <vector>

#include "User.hpp"

// ResponseFormatter: Formats IRC protocol messages according to RFC1459
// All methods are static as this is a stateless formatter
// Message format: :prefix COMMAND params :trailing\r\n
class ResponseFormatter {
 public:
  // ==========================================
  // Welcome messages (001-005)
  // ==========================================
  static std::string rplWelcome(const User* user);
  static std::string rplYourHost(const User* user);
  static std::string rplCreated(const User* user);
  static std::string rplMyInfo(const User* user);

  // ==========================================
  // Command responses
  // ==========================================
  static std::string rplJoin(const User* user, const std::string& channel);
  static std::string rplPart(const User* user, const std::string& channel,
                             const std::string& reason);
  static std::string rplPrivmsg(const User* from, const std::string& target,
                                const std::string& message);
  static std::string rplNotice(const User* from, const std::string& target,
                               const std::string& message);
  static std::string rplTopic(const std::string& channel,
                              const std::string& topic);
  static std::string rplKick(const User* kicker, const std::string& channel,
                             const std::string& kicked,
                             const std::string& reason);
  static std::string rplQuit(const User* user, const std::string& reason);

  // ==========================================
  // Error responses (400-599)
  // ==========================================
  static std::string errNoSuchNick(const std::string& nickname);
  static std::string errNoSuchChannel(const std::string& channel);
  static std::string errCannotSendToChan(const std::string& channel);
  static std::string errTooManyChannels(const std::string& channel);
  static std::string errUnknownCommand(const std::string& command);
  static std::string errErroneusNickname(const std::string& nickname);
  static std::string errNicknameInUse(const std::string& nickname);
  static std::string errNotOnChannel(const std::string& channel);
  static std::string errUserNotInChannel(const std::string& user,
                                         const std::string& channel);
  static std::string errUserOnChannel(const std::string& user,
                                      const std::string& channel);
  static std::string errNeedMoreParams(const std::string& command);
  static std::string errAlreadyRegistered();
  static std::string errPasswdMismatch();
  static std::string errChannelIsFull(const std::string& channel);
  static std::string errInviteOnlyChan(const std::string& channel);
  static std::string errBadChannelKey(const std::string& channel);
  static std::string errChanOPrivsNeeded(const std::string& channel);

 private:
  // Helper: Format IRC message with prefix, command, and parameters
  // Returns: Formatted message with \r\n termination
  static std::string formatMessage(const std::string& prefix,
                                   const std::string& command,
                                   const std::vector<std::string>& params);

  // Helper: Format user prefix (nick!user@host)
  // Returns: Formatted prefix for command responses
  static std::string formatUserPrefix(const User* user);

  ResponseFormatter();                                     // = delete
  ~ResponseFormatter();                                    // = delete
  ResponseFormatter(const ResponseFormatter& src);         // = delete
  ResponseFormatter& operator=(const ResponseFormatter&);  // = delete
};

#endif
