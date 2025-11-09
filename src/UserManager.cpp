#include "UserManager.hpp"

#include "utils.hpp"

UserManager::UserManager() {}

UserManager::~UserManager() { removeAll(); }

void UserManager::addUser(User* user) {
  if (!user) {
    log(LOG_LEVEL_WARNING, LOG_CATEGORY_CONNECTION,
        "Attempted to add NULL user");
    return;
  }

  users_[user->getSocketFd()] = user;

  // Add to nickname index if user has a nickname
  if (!user->getNickname().empty()) {
    usersByNick_[user->getNickname()] = user;
  }
}

void UserManager::removeUser(int fd) {
  std::map<int, User*>::iterator it = users_.find(fd);
  if (it == users_.end()) {
    log(LOG_LEVEL_WARNING, LOG_CATEGORY_CONNECTION,
        "Attempted to remove a non-existent user");
    return;
  }

  User* user = it->second;
  std::string ip = user->getIp();

  // Remove from nickname index
  if (!user->getNickname().empty()) {
    usersByNick_.erase(user->getNickname());
  }

  delete user;  // User destructor closes the socket
  users_.erase(it);

  log(LOG_LEVEL_INFO, LOG_CATEGORY_CONNECTION,
      "User removed successfully: " + ip);
}

void UserManager::removeAll() {
  for (std::map<int, User*>::iterator it = users_.begin(); it != users_.end();
       ++it) {
    delete it->second;  // User destructor closes the socket
  }
  users_.clear();
  usersByNick_.clear();
}

User* UserManager::getUserByFd(int fd) {
  std::map<int, User*>::iterator it = users_.find(fd);
  if (it == users_.end()) return NULL;
  return it->second;
}

User* UserManager::getUserByNickname(const std::string& nickname) {
  std::map<std::string, User*>::iterator it = usersByNick_.find(nickname);
  if (it == usersByNick_.end()) return NULL;
  return it->second;
}

const std::map<int, User*>& UserManager::getUsers() const { return users_; }

bool UserManager::isNicknameInUse(const std::string& nickname) const {
  return usersByNick_.find(nickname) != usersByNick_.end();
}

void UserManager::updateNickname(User* user, const std::string& oldNick,
                                 const std::string& newNick) {
  if (!user) {
    log(LOG_LEVEL_WARNING, LOG_CATEGORY_CONNECTION,
        "Attempted to update nickname for NULL user");
    return;
  }

  // Remove old nickname from index
  if (!oldNick.empty()) {
    usersByNick_.erase(oldNick);
  }

  // Update user's nickname
  user->setNickname(newNick);

  // Add new nickname to index
  if (!newNick.empty()) {
    usersByNick_[newNick] = user;
  }
}
