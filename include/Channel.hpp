/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akyoshid <akyoshid@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/04 00:37:05 by akyoshid          #+#    #+#             */
/*   Updated: 2025/11/04 00:37:45 by akyoshid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef INCLUDE_CHANNEL_HPP_
#define INCLUDE_CHANNEL_HPP_

#include <set>
#include <string>

#include "User.hpp"

// Channel: Represents an IRC channel
// Manages channel members, operators, topic, and modes
class Channel {
 public:
  explicit Channel(const std::string& name);
  ~Channel();

  // Getters
  const std::string& getName() const;
  const std::string& getTopic() const;
  const std::set<int>& getMembers() const;    // Set of user FDs
  const std::set<int>& getOperators() const;  // Set of operator FDs
  bool isInviteOnly() const;
  bool isTopicRestricted() const;
  bool hasUserLimit() const;
  size_t getUserLimit() const;
  const std::string& getKey() const;

  // Setters
  void setTopic(const std::string& topic);
  void setInviteOnly(bool inviteOnly);
  void setTopicRestricted(bool topicRestricted);
  void setUserLimit(size_t limit);
  void clearUserLimit();
  void setKey(const std::string& key);
  void clearKey();

  // Member management
  void addMember(int userFd);
  void removeMember(int userFd);
  bool isMember(int userFd) const;
  size_t getMemberCount() const;

  // Operator management
  void addOperator(int userFd);
  void removeOperator(int userFd);
  bool isOperator(int userFd) const;

  // Invite management
  void addInvite(int userFd);
  void removeInvite(int userFd);
  bool isInvited(int userFd) const;

 private:
  std::string name_;
  std::string topic_;
  std::set<int> members_;    // File descriptors of members
  std::set<int> operators_;  // File descriptors of operators
  std::set<int> invited_;    // File descriptors of invited users

  // Channel modes
  bool inviteOnly_;
  bool topicRestricted_;
  bool hasUserLimit_;
  size_t userLimit_;
  std::string key_;  // Channel password

  Channel();                               // = delete
  Channel(const Channel& src);             // = delete
  Channel& operator=(const Channel& src);  // = delete
};

#endif
