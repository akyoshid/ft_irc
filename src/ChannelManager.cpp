#include "ChannelManager.hpp"

#include "utils.hpp"

ChannelManager::ChannelManager() {}

ChannelManager::~ChannelManager() { removeAll(); }

Channel* ChannelManager::createChannel(const std::string& name) {
  // Normalize channel name (case-insensitive per RFC1459)
  std::string normalizedName = normalizeChannelName(name);

  // Check if channel already exists
  if (channelExists(name)) {
    log(LOG_LEVEL_WARNING, LOG_CATEGORY_CHANNEL,
        "Channel already exists: " + name);
    return NULL;
  }

  // Create new channel with exception safety
  // Store with original name but index with normalized name
  Channel* newChannel = NULL;
  try {
    newChannel = new Channel(name);
    channels_[normalizedName] = newChannel;
  } catch (...) {
    delete newChannel;  // NULL-safe in C++
    throw;
  }

  log(LOG_LEVEL_INFO, LOG_CATEGORY_CHANNEL, "Channel created: " + name);
  return newChannel;
}

void ChannelManager::removeChannel(const std::string& name) {
  std::string normalizedName = normalizeChannelName(name);
  std::map<std::string, Channel*>::iterator it = channels_.find(normalizedName);
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
  std::string normalizedName = normalizeChannelName(name);
  std::map<std::string, Channel*>::iterator it = channels_.find(normalizedName);
  if (it == channels_.end()) return NULL;
  return it->second;
}

const std::map<std::string, Channel*>& ChannelManager::getChannels() const {
  return channels_;
}

bool ChannelManager::channelExists(const std::string& name) const {
  std::string normalizedName = normalizeChannelName(name);
  return channels_.find(normalizedName) != channels_.end();
}
