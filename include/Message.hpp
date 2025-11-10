#ifndef INCLUDE_MESSAGE_HPP_
#define INCLUDE_MESSAGE_HPP_

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class Message {
 public:
  Message(const std::string& msg);
  ~Message(void);

  void print(void) const;

  const std::string& getPrefix(void) const;
  const std::string& getCommand(void) const;
  const std::vector<std::string>& getParams(void) const;

 private:
  std::string prefix_;
  std::string command_;
  std::vector<std::string> params_;

  Message(void);
  Message(const Message& msg);             // = delete
  Message& operator=(const Message& msg);  // = delete

  void parseMessage(const std::string& msg);
  void validate(void) const;
  void validatePrefix(void) const;
  void validateCommand(void) const;
  void validateParams(void) const;
  void consumeSpaces(std::string& str);
};

#endif
