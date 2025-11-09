#ifndef INCLUDE_USERMANAGER_HPP_
#define INCLUDE_USERMANAGER_HPP_

#include <map>
#include <string>

#include "User.hpp"

// UserManager: Manages the collection of connected users
// Handles adding, removing, and looking up users by file descriptor or nickname
class UserManager {
 public:
  UserManager();
  ~UserManager();

  // Add a new user to the manager
  // The UserManager takes ownership of the User object
  void addUser(User* user);

  // Remove a user by file descriptor
  // Deletes the User object and removes from map
  void removeUser(int fd);

  // Remove all users (used in destructor)
  void removeAll();

  // Get a user by file descriptor
  // Returns: Pointer to User, or NULL if not found
  User* getUserByFd(int fd);

  // Get a user by nickname
  // Returns: Pointer to User, or NULL if not found
  User* getUserByNickname(const std::string& nickname);

  // Get all users
  const std::map<int, User*>& getUsers() const;

  // Check if a nickname is already in use
  bool isNicknameInUse(const std::string& nickname) const;

  // Update user's nickname (maintains nickname index)
  // oldNick: Empty string if user has no nickname yet
  // newNick: New nickname to set
  void updateNickname(User* user, const std::string& oldNick,
                      const std::string& newNick);

 private:
  std::map<int, User*> users_;                // fd -> User*
  std::map<std::string, User*> usersByNick_;  // nickname -> User*

  UserManager(const UserManager& src);             // = delete
  UserManager& operator=(const UserManager& src);  // = delete
};

#endif
