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
}

void UserManager::removeUser(int fd) {
  std::map<int, User*>::iterator it = users_.find(fd);
  if (it == users_.end()) {
    log(LOG_LEVEL_WARNING, LOG_CATEGORY_CONNECTION,
        "Attempted to remove a non-existent user");
    return;
  }

  std::string ip = it->second->getIp();
  delete it->second;  // User destructor closes the socket
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
}

User* UserManager::getUserByFd(int fd) {
  std::map<int, User*>::iterator it = users_.find(fd);
  if (it == users_.end()) return NULL;
  return it->second;
}

User* UserManager::getUserByNickname(const std::string& nickname) {
  for (std::map<int, User*>::iterator it = users_.begin(); it != users_.end();
       ++it) {
    if (it->second->getNickname() == nickname) {
      return it->second;
    }
  }
  return NULL;
}

const std::map<int, User*>& UserManager::getUsers() const { return users_; }

bool UserManager::isNicknameInUse(const std::string& nickname) const {
  for (std::map<int, User*>::const_iterator it = users_.begin();
       it != users_.end(); ++it) {
    if (it->second->getNickname() == nickname) {
      return true;
    }
  }
  return false;
}
