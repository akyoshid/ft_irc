#ifndef INCLUDE_MESSAGE_HPP_
#define INCLUDE_MESSAGE_HPP_

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class Message {
 public:
  Message(const std::string& msg);
  ~Message();

  void print() const;

  const std::string& getCommand() const;
  const std::vector<std::string>& getParams() const;
  const std::string& getTrail() const;

 private:
  std::string prefix_;
  std::string command_;
  std::vector<std::string> params_;

  Message();
  Message(const Message& msg);             // = delete
  Message& operator=(const Message& msg);  // = delete

  void parseMessage(const std::string& msg);
  void validate() const;
  void validatePrefix(void) const;
  void validateCommand(void) const;
  void validateParams(void) const;
  void consumeSpaces(std::string& str);
};

#endif
