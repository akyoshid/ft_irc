#include "ChannelManager.hpp"

#include "utils.hpp"

ChannelManager::ChannelManager() {}

ChannelManager::~ChannelManager() { removeAll(); }

Channel* ChannelManager::createChannel(const std::string& name) {
  // Check if channel already exists
  if (channelExists(name)) {
    log(LOG_LEVEL_WARNING, LOG_CATEGORY_CHANNEL,
        "Channel already exists: " + name);
    return NULL;
  }

  // Create new channel with exception safety
  Channel* newChannel = NULL;
  try {
    newChannel = new Channel(name);
    channels_[name] = newChannel;
  } catch (...) {
    delete newChannel;  // NULL-safe in C++
    throw;
  }

  log(LOG_LEVEL_INFO, LOG_CATEGORY_CHANNEL, "Channel created: " + name);
  return newChannel;
}

void ChannelManager::removeChannel(const std::string& name) {
  std::map<std::string, Channel*>::iterator it = channels_.find(name);
  if (it == channels_.end()) {
    log(LOG_LEVEL_WARNING, LOG_CATEGORY_CHANNEL,
        "Attempted to remove non-existent channel: " + name);
    return;
  }

  delete it->second;
  channels_.erase(it);

  log(LOG_LEVEL_INFO, LOG_CATEGORY_CHANNEL, "Channel removed: " + name);
}

void ChannelManager::removeAll() {
  for (std::map<std::string, Channel*>::iterator it = channels_.begin();
       it != channels_.end(); ++it) {
    delete it->second;
  }
  channels_.clear();
}

Channel* ChannelManager::getChannel(const std::string& name) {
  std::map<std::string, Channel*>::iterator it = channels_.find(name);
  if (it == channels_.end()) return NULL;
  return it->second;
}

const std::map<std::string, Channel*>& ChannelManager::getChannels() const {
  return channels_;
}

bool ChannelManager::channelExists(const std::string& name) const {
  return channels_.find(name) != channels_.end();
}
