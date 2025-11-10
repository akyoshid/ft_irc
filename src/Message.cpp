#include "Message.hpp"

const std::string& Message::getPrefix(void) const { return (prefix_); }

const std::string& Message::getCommand(void) const { return (command_); }

const std::vector<std::string>& Message::getParams(void) const {
  return (params_);
}

void Message::parseMessage(const std::string& msg) {
  std::string mutable_msg = msg;

  if (mutable_msg[0] == ':') {
    std::size_t pos = mutable_msg.find(' ');
    if (pos == std::string::npos)
      throw std::runtime_error("Invalid message: Prefix found but no command.");
    prefix_ = mutable_msg.substr(1, pos - 1);
    if (prefix_.empty())
      throw std::runtime_error(
          "Invalid message: Prefix must not be empty or start with space.");
    mutable_msg.erase(0, pos);
    consumeSpaces(mutable_msg);
  }

  if (mutable_msg.empty())
    throw std::runtime_error("Invalid message: No command found.");

  std::size_t pos = mutable_msg.find(' ');
  command_ = mutable_msg.substr(0, pos);
  mutable_msg.erase(0, pos);
  consumeSpaces(mutable_msg);

  while (!mutable_msg.empty()) {
    std::string param;

    if (mutable_msg[0] == ':') {
      param = mutable_msg.substr(1);
      mutable_msg.clear();
    } else {
      std::size_t pos = mutable_msg.find(' ');
      param = mutable_msg.substr(0, pos);
      mutable_msg.erase(0, pos);
    }
    params_.push_back(param);
    consumeSpaces(mutable_msg);
  }
}

void Message::consumeSpaces(std::string& str) {
  if (!str.empty()) {
    std::size_t pos = str.find_first_not_of(' ');
    str.erase(0, pos);
  }
}

void Message::validate(void) const {
  // validatePrefix();
  validateCommand();
  validateParams();
}

// void Message::validatePrefix(void) const {}

void Message::validateCommand(void) const {
  if (command_.empty())
    throw std::runtime_error("Invalid message: No command found.");

  std::string::size_type len = command_.length();

  if (len == 3 && std::isdigit(static_cast<unsigned char>(command_[0])) &&
      std::isdigit(static_cast<unsigned char>(command_[1])) &&
      std::isdigit(static_cast<unsigned char>(command_[2]))) {
    return;
  }

  for (std::string::size_type i = 0; i < len; ++i) {
    if (!std::isalpha(static_cast<unsigned char>(command_[i])))
      throw std::runtime_error(
          "Invalid message: Command must be <letter> { <letter> } | <number> "
          "<number> <number>.");
  }
  return;
}

void Message::validateParams(void) const {
  std::vector<std::string>::size_type paramCount = params_.size();

  if (paramCount == 0) return;
  if (paramCount > 15)
    throw std::runtime_error("Invalid message: Too many params.");

  for (std::vector<std::string>::size_type i = 0; i < paramCount; ++i) {
    const std::string& param = params_[i];

    if (param.find('\0') != std::string::npos)
      throw std::runtime_error("Invalid message: Parameter contains NUL.");

    if (param.find('\r') != std::string::npos)
      throw std::runtime_error("Invalid message: Parameter contains CR.");

    if (param.find('\n') != std::string::npos)
      throw std::runtime_error("Invalid message: Parameter contains LF.");
  }
  return;
}

Message::Message(const std::string& msg) : prefix_(), command_(), params_() {
  if (msg.empty()) throw std::runtime_error("message is empty.");
  if (msg.length() > 510) {
    throw std::runtime_error(
        "510 characters maximum allowed for the command and its parameters");
  }
  if (msg[0] == ' ') {
    throw std::runtime_error(
        "Invalid message: Message must not be empty or start with space.");
  }

  parseMessage(msg);
  validate();
}

Message::~Message(void) {}

void Message::print(void) const {
  std::cout << "Prefix: [" << prefix_ << "]" << std::endl;

  std::cout << "Command: [" << command_ << "]" << std::endl;

  std::cout << "Params:  (" << params_.size() << ")" << std::endl;
  for (std::vector<std::string>::const_iterator it = params_.begin();
       it != params_.end(); ++it) {
    std::cout << "  - [" << *it << "]" << std::endl;
  }
}
