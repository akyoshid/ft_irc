#include "CommandParser.hpp"

#include <stdexcept>
#include <string>

#include "gtest/gtest.h"

// ==========================================
// Test Fixture
// ==========================================
class CommandParserTest : public ::testing::Test {
 protected:
  CommandParser parser;
};

// ==========================================
// Valid Commands - Basic
// ==========================================

TEST_F(CommandParserTest, SimpleCommand) {
  Command cmd = parser.parseCommand("NICK");
  EXPECT_EQ(cmd.prefix, "");
  EXPECT_EQ(cmd.command, "NICK");
  EXPECT_EQ(cmd.params.size(), static_cast<size_t>(0));
}

TEST_F(CommandParserTest, CommandWithOneParam) {
  Command cmd = parser.parseCommand("NICK alice");
  EXPECT_EQ(cmd.prefix, "");
  EXPECT_EQ(cmd.command, "NICK");
  ASSERT_EQ(cmd.params.size(), static_cast<size_t>(1));
  EXPECT_EQ(cmd.params[0], "alice");
}

TEST_F(CommandParserTest, CommandWithMultipleParams) {
  Command cmd = parser.parseCommand("USER alice 0 * Alice");
  EXPECT_EQ(cmd.prefix, "");
  EXPECT_EQ(cmd.command, "USER");
  ASSERT_EQ(cmd.params.size(), static_cast<size_t>(4));
  EXPECT_EQ(cmd.params[0], "alice");
  EXPECT_EQ(cmd.params[1], "0");
  EXPECT_EQ(cmd.params[2], "*");
  EXPECT_EQ(cmd.params[3], "Alice");
}

TEST_F(CommandParserTest, CommandCaseInsensitive) {
  Command cmd1 = parser.parseCommand("nick");
  Command cmd2 = parser.parseCommand("NICK");
  Command cmd3 = parser.parseCommand("NiCk");

  EXPECT_EQ(cmd1.command, "NICK");
  EXPECT_EQ(cmd2.command, "NICK");
  EXPECT_EQ(cmd3.command, "NICK");
}

TEST_F(CommandParserTest, NumericCommand) {
  Command cmd = parser.parseCommand("001 alice :Welcome");
  EXPECT_EQ(cmd.prefix, "");
  EXPECT_EQ(cmd.command, "001");
  ASSERT_EQ(cmd.params.size(), static_cast<size_t>(2));
  EXPECT_EQ(cmd.params[0], "alice");
  EXPECT_EQ(cmd.params[1], "Welcome");
}

// ==========================================
// Trailing Parameters
// ==========================================

TEST_F(CommandParserTest, TrailingParameter) {
  Command cmd = parser.parseCommand("PRIVMSG #channel :Hello World");
  EXPECT_EQ(cmd.command, "PRIVMSG");
  ASSERT_EQ(cmd.params.size(), static_cast<size_t>(2));
  EXPECT_EQ(cmd.params[0], "#channel");
  EXPECT_EQ(cmd.params[1], "Hello World");
}

TEST_F(CommandParserTest, TrailingParameterWithSpaces) {
  Command cmd = parser.parseCommand("PRIVMSG #chan :This has   multiple   spaces");
  ASSERT_EQ(cmd.params.size(), static_cast<size_t>(2));
  EXPECT_EQ(cmd.params[1], "This has   multiple   spaces");
}

TEST_F(CommandParserTest, TrailingParameterEmpty) {
  Command cmd = parser.parseCommand("TOPIC #channel :");
  ASSERT_EQ(cmd.params.size(), static_cast<size_t>(2));
  EXPECT_EQ(cmd.params[0], "#channel");
  EXPECT_EQ(cmd.params[1], "");
}

TEST_F(CommandParserTest, TrailingParameterWithColon) {
  Command cmd = parser.parseCommand("PRIVMSG #chan ::-)");
  ASSERT_EQ(cmd.params.size(), static_cast<size_t>(2));
  EXPECT_EQ(cmd.params[1], ":-)");
}

// ==========================================
// Prefix Handling
// ==========================================

TEST_F(CommandParserTest, CommandWithPrefix) {
  Command cmd = parser.parseCommand(":server.com NOTICE alice :Welcome");
  EXPECT_EQ(cmd.prefix, "server.com");
  EXPECT_EQ(cmd.command, "NOTICE");
  ASSERT_EQ(cmd.params.size(), static_cast<size_t>(2));
  EXPECT_EQ(cmd.params[0], "alice");
  EXPECT_EQ(cmd.params[1], "Welcome");
}

TEST_F(CommandParserTest, CommandWithUserPrefix) {
  Command cmd = parser.parseCommand(":alice!user@host PRIVMSG bob :Hi");
  EXPECT_EQ(cmd.prefix, "alice!user@host");
  EXPECT_EQ(cmd.command, "PRIVMSG");
  ASSERT_EQ(cmd.params.size(), static_cast<size_t>(2));
  EXPECT_EQ(cmd.params[0], "bob");
  EXPECT_EQ(cmd.params[1], "Hi");
}

// ==========================================
// Whitespace Handling
// ==========================================

TEST_F(CommandParserTest, MultipleSpacesBetweenParams) {
  Command cmd = parser.parseCommand("JOIN    #channel    key");
  EXPECT_EQ(cmd.command, "JOIN");
  ASSERT_EQ(cmd.params.size(), static_cast<size_t>(2));
  EXPECT_EQ(cmd.params[0], "#channel");
  EXPECT_EQ(cmd.params[1], "key");
}

TEST_F(CommandParserTest, SpacesAfterPrefix) {
  Command cmd = parser.parseCommand(":prefix    COMMAND param");
  EXPECT_EQ(cmd.prefix, "prefix");
  EXPECT_EQ(cmd.command, "COMMAND");
  ASSERT_EQ(cmd.params.size(), static_cast<size_t>(1));
  EXPECT_EQ(cmd.params[0], "param");
}

TEST_F(CommandParserTest, TrailingSpacesIgnored) {
  Command cmd = parser.parseCommand("NICK alice   ");
  EXPECT_EQ(cmd.command, "NICK");
  ASSERT_EQ(cmd.params.size(), static_cast<size_t>(1));
  EXPECT_EQ(cmd.params[0], "alice");
}

// ==========================================
// Maximum Parameters (15 max per RFC1459)
// ==========================================

TEST_F(CommandParserTest, MaximumParameters) {
  Command cmd = parser.parseCommand(
      "CMD p1 p2 p3 p4 p5 p6 p7 p8 p9 p10 p11 p12 p13 p14 p15");
  EXPECT_EQ(cmd.command, "CMD");
  EXPECT_EQ(cmd.params.size(), static_cast<size_t>(15));
  EXPECT_EQ(cmd.params[0], "p1");
  EXPECT_EQ(cmd.params[14], "p15");
}

// ==========================================
// Error Cases - Empty/Invalid
// ==========================================

TEST_F(CommandParserTest, EmptyMessage) {
  EXPECT_THROW(parser.parseCommand(""), std::runtime_error);
}

TEST_F(CommandParserTest, MessageStartsWithSpace) {
  EXPECT_THROW(parser.parseCommand(" NICK alice"), std::runtime_error);
}

TEST_F(CommandParserTest, EmptyPrefix) {
  EXPECT_THROW(parser.parseCommand(": NICK alice"), std::runtime_error);
}

TEST_F(CommandParserTest, PrefixWithoutCommand) {
  EXPECT_THROW(parser.parseCommand(":prefix"), std::runtime_error);
}

TEST_F(CommandParserTest, PrefixWithOnlySpaces) {
  EXPECT_THROW(parser.parseCommand(":prefix   "), std::runtime_error);
}

// ==========================================
// Error Cases - Invalid Command
// ==========================================

TEST_F(CommandParserTest, CommandWithNumbers) {
  EXPECT_THROW(parser.parseCommand("NICK123 alice"), std::runtime_error);
}

TEST_F(CommandParserTest, CommandWithSpecialChars) {
  EXPECT_THROW(parser.parseCommand("NI-CK alice"), std::runtime_error);
}

TEST_F(CommandParserTest, NumericCommandTooShort) {
  EXPECT_THROW(parser.parseCommand("01 alice"), std::runtime_error);
}

TEST_F(CommandParserTest, NumericCommandTooLong) {
  EXPECT_THROW(parser.parseCommand("0012 alice"), std::runtime_error);
}

TEST_F(CommandParserTest, NumericCommandPartialDigits) {
  EXPECT_THROW(parser.parseCommand("01A alice"), std::runtime_error);
}

// ==========================================
// Error Cases - Too Many Parameters
// ==========================================

TEST_F(CommandParserTest, TooManyParameters) {
  EXPECT_THROW(
      parser.parseCommand(
          "CMD p1 p2 p3 p4 p5 p6 p7 p8 p9 p10 p11 p12 p13 p14 p15 p16"),
      std::runtime_error);
}

// ==========================================
// Error Cases - Length Limit
// ==========================================

TEST_F(CommandParserTest, MessageTooLong) {
  std::string longMessage(511, 'A');
  EXPECT_THROW(parser.parseCommand(longMessage), std::runtime_error);
}

TEST_F(CommandParserTest, MessageExactly510Chars) {
  // 510 chars is the maximum allowed
  std::string message = "NICK ";
  message.append(505, 'a');  // 5 + 505 = 510
  Command cmd = parser.parseCommand(message);
  EXPECT_EQ(cmd.command, "NICK");
}

// ==========================================
// Error Cases - Invalid Characters
// ==========================================

TEST_F(CommandParserTest, ParameterWithNUL) {
  std::string message = "NICK alice";
  message[7] = '\0';  // Inject NUL character
  // Note: This test may need adjustment depending on how the parser handles
  // embedded NULs
  EXPECT_THROW(parser.parseCommand(message), std::runtime_error);
}

TEST_F(CommandParserTest, ParameterWithCR) {
  EXPECT_THROW(parser.parseCommand("NICK ali\rce"), std::runtime_error);
}

TEST_F(CommandParserTest, ParameterWithLF) {
  EXPECT_THROW(parser.parseCommand("NICK ali\nce"), std::runtime_error);
}

// ==========================================
// Real-World IRC Commands
// ==========================================

TEST_F(CommandParserTest, PassCommand) {
  Command cmd = parser.parseCommand("PASS secret");
  EXPECT_EQ(cmd.command, "PASS");
  ASSERT_EQ(cmd.params.size(), static_cast<size_t>(1));
  EXPECT_EQ(cmd.params[0], "secret");
}

TEST_F(CommandParserTest, UserCommand) {
  Command cmd = parser.parseCommand("USER alice 0 * :Alice Smith");
  EXPECT_EQ(cmd.command, "USER");
  ASSERT_EQ(cmd.params.size(), static_cast<size_t>(4));
  EXPECT_EQ(cmd.params[0], "alice");
  EXPECT_EQ(cmd.params[1], "0");
  EXPECT_EQ(cmd.params[2], "*");
  EXPECT_EQ(cmd.params[3], "Alice Smith");
}

TEST_F(CommandParserTest, JoinCommand) {
  Command cmd = parser.parseCommand("JOIN #channel");
  EXPECT_EQ(cmd.command, "JOIN");
  ASSERT_EQ(cmd.params.size(), static_cast<size_t>(1));
  EXPECT_EQ(cmd.params[0], "#channel");
}

TEST_F(CommandParserTest, JoinCommandWithKey) {
  Command cmd = parser.parseCommand("JOIN #private secretkey");
  EXPECT_EQ(cmd.command, "JOIN");
  ASSERT_EQ(cmd.params.size(), static_cast<size_t>(2));
  EXPECT_EQ(cmd.params[0], "#private");
  EXPECT_EQ(cmd.params[1], "secretkey");
}

TEST_F(CommandParserTest, PrivmsgCommand) {
  Command cmd = parser.parseCommand("PRIVMSG #general :Hello everyone!");
  EXPECT_EQ(cmd.command, "PRIVMSG");
  ASSERT_EQ(cmd.params.size(), static_cast<size_t>(2));
  EXPECT_EQ(cmd.params[0], "#general");
  EXPECT_EQ(cmd.params[1], "Hello everyone!");
}

TEST_F(CommandParserTest, ModeCommand) {
  Command cmd = parser.parseCommand("MODE #channel +o alice");
  EXPECT_EQ(cmd.command, "MODE");
  ASSERT_EQ(cmd.params.size(), static_cast<size_t>(3));
  EXPECT_EQ(cmd.params[0], "#channel");
  EXPECT_EQ(cmd.params[1], "+o");
  EXPECT_EQ(cmd.params[2], "alice");
}

TEST_F(CommandParserTest, KickCommand) {
  Command cmd = parser.parseCommand("KICK #channel alice :Bad behavior");
  EXPECT_EQ(cmd.command, "KICK");
  ASSERT_EQ(cmd.params.size(), static_cast<size_t>(3));
  EXPECT_EQ(cmd.params[0], "#channel");
  EXPECT_EQ(cmd.params[1], "alice");
  EXPECT_EQ(cmd.params[2], "Bad behavior");
}

TEST_F(CommandParserTest, TopicCommand) {
  Command cmd = parser.parseCommand("TOPIC #channel :Welcome to our channel!");
  EXPECT_EQ(cmd.command, "TOPIC");
  ASSERT_EQ(cmd.params.size(), static_cast<size_t>(2));
  EXPECT_EQ(cmd.params[0], "#channel");
  EXPECT_EQ(cmd.params[1], "Welcome to our channel!");
}

TEST_F(CommandParserTest, PingCommand) {
  Command cmd = parser.parseCommand("PING :server1");
  EXPECT_EQ(cmd.command, "PING");
  ASSERT_EQ(cmd.params.size(), static_cast<size_t>(1));
  EXPECT_EQ(cmd.params[0], "server1");
}

TEST_F(CommandParserTest, QuitCommand) {
  Command cmd = parser.parseCommand("QUIT :Goodbye!");
  EXPECT_EQ(cmd.command, "QUIT");
  ASSERT_EQ(cmd.params.size(), static_cast<size_t>(1));
  EXPECT_EQ(cmd.params[0], "Goodbye!");
}

// ==========================================
// Edge Cases
// ==========================================

TEST_F(CommandParserTest, CommandOnly) {
  Command cmd = parser.parseCommand("PING");
  EXPECT_EQ(cmd.command, "PING");
  EXPECT_EQ(cmd.params.size(), static_cast<size_t>(0));
}

TEST_F(CommandParserTest, TrailingWithLeadingSpaces) {
  Command cmd = parser.parseCommand("PRIVMSG #chan :  message with leading spaces");
  ASSERT_EQ(cmd.params.size(), static_cast<size_t>(2));
  EXPECT_EQ(cmd.params[1], "  message with leading spaces");
}

TEST_F(CommandParserTest, ColonInRegularParam) {
  // Colon must be at start of param to indicate trailing
  Command cmd = parser.parseCommand("CMD param:with:colons");
  ASSERT_EQ(cmd.params.size(), static_cast<size_t>(1));
  EXPECT_EQ(cmd.params[0], "param:with:colons");
}
