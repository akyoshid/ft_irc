#include "CommandRouter.hpp"

#include <string>

#include "ChannelManager.hpp"
#include "EventLoop.hpp"
#include "UserManager.hpp"
#include "gtest/gtest.h"

// ==========================================
// Test Fixture with CommandRouter instance
// ==========================================

class ValidationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create dependencies for CommandRouter
    userManager = new UserManager();
    channelManager = new ChannelManager();
    eventLoop = new EventLoop();
    // Create CommandRouter instance
    router = new CommandRouter(userManager, channelManager, eventLoop, "test");
  }

  void TearDown() override {
    delete router;
    delete eventLoop;
    delete channelManager;
    delete userManager;
  }

  UserManager* userManager;
  ChannelManager* channelManager;
  EventLoop* eventLoop;
  CommandRouter* router;
};

// Note: Since isValidChannelName and isValidNickname are private methods,
// we test them indirectly through the public API by observing error responses.
// However, for pure validation logic, the following approach uses extracted
// validation that mirrors CommandRouter's implementation.

// Since we cannot directly call private methods, we create a test-friendly
// approach: We verify validation through command processing behavior.
// Alternative: Make validation functions public or extract to utils.

// For now, we replicate the validation logic for unit testing.
// In production code, consider extracting validation to a public utility class.

// ==========================================
// Validation Helper Functions (from CommandRouter)
// ==========================================

bool isValidChannelName(const std::string& name) {
  // Note: This implementation is more lenient than strict RFC1459
  // RFC1459: <channel> ::= ('#' | '&') <chstring> (requires at least 1 char after prefix)
  // Our implementation: allows "#" or "&" alone (compatible with servers like ngircd)
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

bool isValidNickname(const std::string& nickname) {
  if (nickname.empty() || nickname.length() > 9) {
    return false;
  }

  // First character must be a letter
  if (!std::isalpha(static_cast<unsigned char>(nickname[0]))) {
    return false;
  }

  // Rest can be letters, digits, or special characters (-, [, ], \, `, ^, {, })
  for (size_t i = 1; i < nickname.length(); ++i) {
    char c = nickname[i];
    if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '[' &&
        c != ']' && c != '\\' && c != '`' && c != '^' && c != '{' && c != '}') {
      return false;
    }
  }

  return true;
}

// ==========================================
// Channel Name Validation Tests
// ==========================================

TEST_F(ValidationTest, ValidChannelName_Basic) {
  EXPECT_TRUE(isValidChannelName("#general"));
  EXPECT_TRUE(isValidChannelName("#test"));
  EXPECT_TRUE(isValidChannelName("#random"));
}

TEST_F(ValidationTest, ValidChannelName_WithHyphen) {
  EXPECT_TRUE(isValidChannelName("#test-channel"));
  EXPECT_TRUE(isValidChannelName("#my-room"));
}

TEST_F(ValidationTest, ValidChannelName_WithUnderscore) {
  EXPECT_TRUE(isValidChannelName("#test_channel"));
  EXPECT_TRUE(isValidChannelName("#my_room"));
}

TEST_F(ValidationTest, ValidChannelName_WithNumbers) {
  EXPECT_TRUE(isValidChannelName("#channel123"));
  EXPECT_TRUE(isValidChannelName("#42"));
}

TEST_F(ValidationTest, ValidChannelName_AmpersandPrefix) {
  EXPECT_TRUE(isValidChannelName("&local"));
  EXPECT_TRUE(isValidChannelName("&test"));
}

TEST_F(ValidationTest, ValidChannelName_MaxLength) {
  std::string longName = "#";
  longName.append(199, 'a');  // 1 + 199 = 200 (max length)
  EXPECT_TRUE(isValidChannelName(longName));
}

TEST_F(ValidationTest, InvalidChannelName_Empty) {
  EXPECT_FALSE(isValidChannelName(""));
}

TEST_F(ValidationTest, InvalidChannelName_NoPrefix) {
  EXPECT_FALSE(isValidChannelName("general"));
  EXPECT_FALSE(isValidChannelName("test"));
}

TEST_F(ValidationTest, ValidChannelName_OnlyHash) {
  // Note: Strict RFC1459 requires at least one char after prefix
  // Our implementation is more lenient (compatible with ngircd)
  // This allows "#" or "&" alone, prioritizing practical use over strict RFC
  EXPECT_TRUE(isValidChannelName("#"));
  EXPECT_TRUE(isValidChannelName("&"));
}

TEST_F(ValidationTest, InvalidChannelName_WithSpace) {
  EXPECT_FALSE(isValidChannelName("#test channel"));
  EXPECT_FALSE(isValidChannelName("#test "));
  EXPECT_FALSE(isValidChannelName("# test"));
}

TEST_F(ValidationTest, InvalidChannelName_WithComma) {
  EXPECT_FALSE(isValidChannelName("#test,channel"));
  EXPECT_FALSE(isValidChannelName("#test,"));
}

TEST_F(ValidationTest, InvalidChannelName_WithBell) {
  std::string withBell = "#test";
  withBell += static_cast<char>(7);  // BELL character
  EXPECT_FALSE(isValidChannelName(withBell));
}

TEST_F(ValidationTest, InvalidChannelName_TooLong) {
  std::string tooLong = "#";
  tooLong.append(200, 'a');  // 1 + 200 = 201 (exceeds max)
  EXPECT_FALSE(isValidChannelName(tooLong));
}

TEST_F(ValidationTest, InvalidChannelName_WrongPrefix) {
  EXPECT_FALSE(isValidChannelName("@channel"));
  EXPECT_FALSE(isValidChannelName("+channel"));
  EXPECT_FALSE(isValidChannelName("!channel"));
}

// ==========================================
// Nickname Validation Tests
// ==========================================

TEST_F(ValidationTest, ValidNickname_Basic) {
  EXPECT_TRUE(isValidNickname("alice"));
  EXPECT_TRUE(isValidNickname("Bob"));
  EXPECT_TRUE(isValidNickname("Charlie"));
}

TEST_F(ValidationTest, ValidNickname_WithNumbers) {
  EXPECT_TRUE(isValidNickname("alice123"));
  EXPECT_TRUE(isValidNickname("user42"));
}

TEST_F(ValidationTest, ValidNickname_WithSpecialChars) {
  EXPECT_TRUE(isValidNickname("alice-"));
  EXPECT_TRUE(isValidNickname("bob["));
  EXPECT_TRUE(isValidNickname("user]"));
  EXPECT_TRUE(isValidNickname("test\\"));
  EXPECT_TRUE(isValidNickname("name`"));
  EXPECT_TRUE(isValidNickname("user^"));
  EXPECT_TRUE(isValidNickname("test{"));
  EXPECT_TRUE(isValidNickname("name}"));
}

TEST_F(ValidationTest, ValidNickname_MaxLength) {
  EXPECT_TRUE(isValidNickname("alice1234"));  // 9 chars (max)
  EXPECT_TRUE(isValidNickname("abcdefghi"));  // 9 chars
}

TEST_F(ValidationTest, ValidNickname_SingleChar) {
  EXPECT_TRUE(isValidNickname("a"));
  EXPECT_TRUE(isValidNickname("Z"));
}

TEST_F(ValidationTest, InvalidNickname_Empty) {
  EXPECT_FALSE(isValidNickname(""));
}

TEST_F(ValidationTest, InvalidNickname_TooLong) {
  EXPECT_FALSE(isValidNickname("alice12345"));  // 10 chars (exceeds max)
  EXPECT_FALSE(isValidNickname("verylongname"));
}

TEST_F(ValidationTest, InvalidNickname_StartsWithNumber) {
  EXPECT_FALSE(isValidNickname("1alice"));
  EXPECT_FALSE(isValidNickname("42user"));
}

TEST_F(ValidationTest, InvalidNickname_StartsWithHyphen) {
  EXPECT_FALSE(isValidNickname("-alice"));
}

TEST_F(ValidationTest, InvalidNickname_StartsWithSpecialChar) {
  EXPECT_FALSE(isValidNickname("[alice"));
  EXPECT_FALSE(isValidNickname("_user"));  // underscore not allowed per RFC1459
}

TEST_F(ValidationTest, InvalidNickname_WithUnderscore) {
  // RFC1459 strict mode: underscore not allowed
  EXPECT_FALSE(isValidNickname("alice_bob"));
  EXPECT_FALSE(isValidNickname("user_"));
}

TEST_F(ValidationTest, InvalidNickname_WithSpace) {
  EXPECT_FALSE(isValidNickname("alice bob"));
  EXPECT_FALSE(isValidNickname("user "));
}

TEST_F(ValidationTest, InvalidNickname_WithAt) {
  EXPECT_FALSE(isValidNickname("alice@host"));
  EXPECT_FALSE(isValidNickname("user@"));
}

TEST_F(ValidationTest, InvalidNickname_WithExclamation) {
  EXPECT_FALSE(isValidNickname("alice!"));
  EXPECT_FALSE(isValidNickname("user!host"));
}

TEST_F(ValidationTest, InvalidNickname_WithDot) {
  EXPECT_FALSE(isValidNickname("alice.bob"));
  EXPECT_FALSE(isValidNickname("user."));
}

// ==========================================
// Edge Cases
// ==========================================

TEST_F(ValidationTest, ChannelName_SpecialButValid) {
  EXPECT_TRUE(isValidChannelName("#test-channel_123"));
  EXPECT_TRUE(isValidChannelName("#CaseSensitive"));
  EXPECT_TRUE(isValidChannelName("#c"));  // single char after #
}

TEST_F(ValidationTest, Nickname_AllSpecialChars) {
  EXPECT_TRUE(isValidNickname("a[]\\`^{}"));  // letter + all allowed specials
  EXPECT_TRUE(isValidNickname("a-"));
}

TEST_F(ValidationTest, Nickname_MixedValid) {
  EXPECT_TRUE(isValidNickname("a1b2c3"));
  EXPECT_TRUE(isValidNickname("abc123"));
  EXPECT_TRUE(isValidNickname("User-42"));
}

// ==========================================
// RFC1459 Compliance
// ==========================================

TEST_F(ValidationTest, RFC1459_NicknameMaxLength) {
  // RFC1459 specifies 9 character max
  EXPECT_TRUE(isValidNickname("ninechars"));   // exactly 9
  EXPECT_FALSE(isValidNickname("tencharss1"));  // 10 chars (was counting wrong)
}

TEST_F(ValidationTest, RFC1459_ChannelMaxLength) {
  // RFC1459 specifies 200 character max
  std::string exact200 = "#";
  exact200.append(199, 'x');
  EXPECT_TRUE(isValidChannelName(exact200));

  std::string over200 = "#";
  over200.append(200, 'x');
  EXPECT_FALSE(isValidChannelName(over200));
}

TEST_F(ValidationTest, RFC1459_NicknameSpecialChars) {
  // RFC1459 allows: letter [ letters / digits / special ]
  // special = '-' / '[' / ']' / '\' / '`' / '^' / '{' / '}'
  EXPECT_TRUE(isValidNickname("test-"));
  EXPECT_TRUE(isValidNickname("test["));
  EXPECT_TRUE(isValidNickname("test]"));
  EXPECT_TRUE(isValidNickname("test\\"));
  EXPECT_TRUE(isValidNickname("test`"));
  EXPECT_TRUE(isValidNickname("test^"));
  EXPECT_TRUE(isValidNickname("test{"));
  EXPECT_TRUE(isValidNickname("test}"));
}
