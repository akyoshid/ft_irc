#include "utils.hpp"

#include <string>

#include "gtest/gtest.h"

// ==========================================
// normalizeNickname Tests
// ==========================================

TEST(UtilsTest, NormalizeNickname_Basic) {
  EXPECT_EQ(normalizeNickname("Alice"), "alice");
  EXPECT_EQ(normalizeNickname("BOB"), "bob");
  EXPECT_EQ(normalizeNickname("Charlie"), "charlie");
}

TEST(UtilsTest, NormalizeNickname_AlreadyLowercase) {
  EXPECT_EQ(normalizeNickname("alice"), "alice");
  EXPECT_EQ(normalizeNickname("bob"), "bob");
}

TEST(UtilsTest, NormalizeNickname_MixedCase) {
  EXPECT_EQ(normalizeNickname("AlIcE"), "alice");
  EXPECT_EQ(normalizeNickname("bOb"), "bob");
  EXPECT_EQ(normalizeNickname("ChArLiE"), "charlie");
}

TEST(UtilsTest, NormalizeNickname_WithSpecialChars) {
  // IRC allows special chars in nicknames: []{}|^_-`
  EXPECT_EQ(normalizeNickname("Alice[]"), "alice[]");
  EXPECT_EQ(normalizeNickname("Bob-123"), "bob-123");
  EXPECT_EQ(normalizeNickname("User_Name"), "user_name");
  EXPECT_EQ(normalizeNickname("Test|User"), "test|user");
}

TEST(UtilsTest, NormalizeNickname_WithNumbers) {
  EXPECT_EQ(normalizeNickname("Alice123"), "alice123");
  EXPECT_EQ(normalizeNickname("User42"), "user42");
}

TEST(UtilsTest, NormalizeNickname_Empty) {
  EXPECT_EQ(normalizeNickname(""), "");
}

TEST(UtilsTest, NormalizeNickname_SingleChar) {
  EXPECT_EQ(normalizeNickname("A"), "a");
  EXPECT_EQ(normalizeNickname("Z"), "z");
}

TEST(UtilsTest, NormalizeNickname_AllUppercase) {
  EXPECT_EQ(normalizeNickname("ALICE"), "alice");
  EXPECT_EQ(normalizeNickname("LONGERNAME"), "longername");
}

TEST(UtilsTest, NormalizeNickname_Idempotent) {
  // Normalizing twice should give same result
  std::string once = normalizeNickname("Alice");
  std::string twice = normalizeNickname(once);
  EXPECT_EQ(once, twice);
  EXPECT_EQ(once, "alice");
}

// ==========================================
// normalizeChannelName Tests
// ==========================================

TEST(UtilsTest, NormalizeChannelName_Basic) {
  EXPECT_EQ(normalizeChannelName("#General"), "#general");
  EXPECT_EQ(normalizeChannelName("#RANDOM"), "#random");
  EXPECT_EQ(normalizeChannelName("#test"), "#test");
}

TEST(UtilsTest, NormalizeChannelName_AlreadyLowercase) {
  EXPECT_EQ(normalizeChannelName("#general"), "#general");
  EXPECT_EQ(normalizeChannelName("#random"), "#random");
}

TEST(UtilsTest, NormalizeChannelName_MixedCase) {
  EXPECT_EQ(normalizeChannelName("#GeNeRaL"), "#general");
  EXPECT_EQ(normalizeChannelName("#RaNdOm"), "#random");
  EXPECT_EQ(normalizeChannelName("#TeSt"), "#test");
}

TEST(UtilsTest, NormalizeChannelName_WithHyphenUnderscore) {
  EXPECT_EQ(normalizeChannelName("#test-channel"), "#test-channel");
  EXPECT_EQ(normalizeChannelName("#my_channel"), "#my_channel");
  EXPECT_EQ(normalizeChannelName("#Channel-Name_123"), "#channel-name_123");
}

TEST(UtilsTest, NormalizeChannelName_WithNumbers) {
  EXPECT_EQ(normalizeChannelName("#channel42"), "#channel42");
  EXPECT_EQ(normalizeChannelName("#123test"), "#123test");
}

TEST(UtilsTest, NormalizeChannelName_Empty) {
  EXPECT_EQ(normalizeChannelName(""), "");
}

TEST(UtilsTest, NormalizeChannelName_OnlyHash) {
  EXPECT_EQ(normalizeChannelName("#"), "#");
}

TEST(UtilsTest, NormalizeChannelName_AllUppercase) {
  EXPECT_EQ(normalizeChannelName("#GENERAL"), "#general");
  EXPECT_EQ(normalizeChannelName("#LONGCHANNELNAME"), "#longchannelname");
}

TEST(UtilsTest, NormalizeChannelName_Idempotent) {
  // Normalizing twice should give same result
  std::string once = normalizeChannelName("#General");
  std::string twice = normalizeChannelName(once);
  EXPECT_EQ(once, twice);
  EXPECT_EQ(once, "#general");
}

// ==========================================
// Case Insensitivity Verification
// ==========================================

TEST(UtilsTest, NormalizationMakesEqual) {
  // Different cases should normalize to same value
  EXPECT_EQ(normalizeNickname("Alice"), normalizeNickname("alice"));
  EXPECT_EQ(normalizeNickname("Alice"), normalizeNickname("ALICE"));
  EXPECT_EQ(normalizeNickname("Alice"), normalizeNickname("aLiCe"));

  EXPECT_EQ(normalizeChannelName("#Channel"), normalizeChannelName("#channel"));
  EXPECT_EQ(normalizeChannelName("#Channel"),
            normalizeChannelName("#CHANNEL"));
  EXPECT_EQ(normalizeChannelName("#Channel"),
            normalizeChannelName("#ChAnNeL"));
}

// ==========================================
// int_to_string Tests
// ==========================================

TEST(UtilsTest, IntToString_Zero) { EXPECT_EQ(int_to_string(0), "0"); }

TEST(UtilsTest, IntToString_PositiveNumbers) {
  EXPECT_EQ(int_to_string(1), "1");
  EXPECT_EQ(int_to_string(42), "42");
  EXPECT_EQ(int_to_string(123), "123");
  EXPECT_EQ(int_to_string(9999), "9999");
}

TEST(UtilsTest, IntToString_NegativeNumbers) {
  EXPECT_EQ(int_to_string(-1), "-1");
  EXPECT_EQ(int_to_string(-42), "-42");
  EXPECT_EQ(int_to_string(-123), "-123");
}

TEST(UtilsTest, IntToString_LargeNumbers) {
  EXPECT_EQ(int_to_string(2147483647), "2147483647");   // INT_MAX
  EXPECT_EQ(int_to_string(-2147483648), "-2147483648");  // INT_MIN (approx)
}
