#ifndef INCLUDE_MESSAGE_HPP_
#define INCLUDE_MESSAGE_HPP_

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class message {
 public:
  message(std::string mes);
  ~message();

  void print() const;

  const std::string& getCommand() const;
  const std::vector<std::string>& getParams() const;
  const std::string& getTrail() const;

 private:
  std::string command;
  std::vector<std::string> params;
  std::string trail;

  message();

  static void ltrim(std::string& s, const std::string& chars = " ");
  static void rtrim(std::string& s, const std::string& chars = " ");
  static void trim(std::string& s, const std::string& chars = " ");
};

#endif
