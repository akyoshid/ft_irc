#include "Message.hpp"

const std::string WHITESPACE = " ";

const std::string& Message::getCommand() const { return (command); }

const std::vector<std::string>& Message::getParams() const { return (params); }

const std::string& Message::getTrail() const { return (trail); }

void Message::ltrim(std::string& s, const std::string& chars) {
  std::string::size_type pos = s.find_first_not_of(chars);
  if (pos == std::string::npos) {
    s.clear();
  } else {
    s.erase(0, pos);
  }
}

void Message::rtrim(std::string& s, const std::string& chars) {
  std::string::size_type pos = s.find_last_not_of(chars);
  if (pos == std::string::npos) {
    s.clear();
  } else {
    s.erase(pos + 1);
  }
}

void Message::trim(std::string& s, const std::string& chars) {
  rtrim(s, chars);
  ltrim(s, chars);
}

Message::Message(std::string mes) : command(), params(), trail() {
  std::string::size_type pos = mes.find(" :");
  if (pos != std::string::npos) {
    trail = mes.substr(pos + 2, std::string::npos);
    mes.erase(pos);
    trim(trail, WHITESPACE);
  }

  trim(mes, WHITESPACE);

  std::stringstream ss(mes);

  if (ss >> command) {
    std::string param;
    while (ss >> param) {
      params.push_back(param);
    }
  }
}

Message::~Message() {}

void Message::print() const {
  std::cout << "Command: [" << command << "]" << std::endl;

  std::cout << "Params:  (" << params.size() << ")" << std::endl;
  for (std::vector<std::string>::const_iterator it = params.begin();
       it != params.end(); ++it) {
    std::cout << "  - [" << *it << "]" << std::endl;
  }

  std::cout << "Trail:   [" << trail << "]" << std::endl;
}
