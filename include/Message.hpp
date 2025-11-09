#ifndef MESSAGE_H
#define MESSAGE_H

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class message {
 public:
  message(std::string mes);
  ~message();

  void print() const;

  std::string command;
  std::vector<std::string> params;
  std::string trail;

 private:
  message();

  static void ltrim(std::string& s, const std::string& chars = " \t\n\r\f\v");
  static void rtrim(std::string& s, const std::string& chars = " \t\n\r\f\v");
  static void trim(std::string& s, const std::string& chars = " \t\n\r\f\v");
};

#endif
