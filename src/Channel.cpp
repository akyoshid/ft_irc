/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: reasuke <reasuke@student.42tokyo.jp>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/04 00:37:03 by akyoshid          #+#    #+#             */
/*   Updated: 2025/11/08 18:00:00 by reasuke          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Channel.hpp"

Channel::Channel(const std::string& name)
    : name_(name),
      inviteOnly_(false),
      topicRestricted_(true),
      hasUserLimit_(false),
      userLimit_(0) {}

Channel::~Channel() {}

// Getters
const std::string& Channel::getName() const { return name_; }

const std::string& Channel::getTopic() const { return topic_; }

const std::set<int>& Channel::getMembers() const { return members_; }

const std::set<int>& Channel::getOperators() const { return operators_; }

bool Channel::isInviteOnly() const { return inviteOnly_; }

bool Channel::isTopicRestricted() const { return topicRestricted_; }

bool Channel::hasUserLimit() const { return hasUserLimit_; }

size_t Channel::getUserLimit() const { return userLimit_; }

const std::string& Channel::getKey() const { return key_; }

// Setters
void Channel::setTopic(const std::string& topic) { topic_ = topic; }

void Channel::setInviteOnly(bool inviteOnly) { inviteOnly_ = inviteOnly; }

void Channel::setTopicRestricted(bool topicRestricted) {
  topicRestricted_ = topicRestricted;
}

void Channel::setUserLimit(size_t limit) {
  hasUserLimit_ = true;
  userLimit_ = limit;
}

void Channel::clearUserLimit() {
  hasUserLimit_ = false;
  userLimit_ = 0;
}

void Channel::setKey(const std::string& key) { key_ = key; }

void Channel::clearKey() { key_ = ""; }

// Member management
void Channel::addMember(int userFd) { members_.insert(userFd); }

void Channel::removeMember(int userFd) {
  members_.erase(userFd);
  operators_.erase(userFd);
  invited_.erase(userFd);
}

bool Channel::isMember(int userFd) const {
  return members_.find(userFd) != members_.end();
}

size_t Channel::getMemberCount() const { return members_.size(); }

// Operator management
void Channel::addOperator(int userFd) { operators_.insert(userFd); }

void Channel::removeOperator(int userFd) { operators_.erase(userFd); }

bool Channel::isOperator(int userFd) const {
  return operators_.find(userFd) != operators_.end();
}

// Invite management
void Channel::addInvite(int userFd) { invited_.insert(userFd); }

void Channel::removeInvite(int userFd) { invited_.erase(userFd); }

bool Channel::isInvited(int userFd) const {
  return invited_.find(userFd) != invited_.end();
}
