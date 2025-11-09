#include "Message.hpp"

int main(void) {
  try {
    std::string str = "NICK Nick\nname";
    std::cout << str << std::endl;
    Message msg(str);
    msg.print();
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
  return (0);
}
