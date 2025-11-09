#ifndef INCLUDE_CHANNELMANAGER_HPP_
#define INCLUDE_CHANNELMANAGER_HPP_

#include <map>
#include <string>

#include "Channel.hpp"

// ChannelManager: Manages the collection of IRC channels
// Handles creating, removing, and looking up channels by name
class ChannelManager {
 public:
  ChannelManager();
  ~ChannelManager();

  // Create a new channel
  // Returns: Pointer to the created channel, or NULL if channel already exists
  Channel* createChannel(const std::string& name);

  // Remove a channel by name
  // The channel is deleted if it exists
  void removeChannel(const std::string& name);

  // Remove all channels (used in destructor)
  void removeAll();

  // Get a channel by name
  // Returns: Pointer to Channel, or NULL if not found
  Channel* getChannel(const std::string& name);

  // Get all channels
  const std::map<std::string, Channel*>& getChannels() const;

  // Check if a channel exists
  bool channelExists(const std::string& name) const;

 private:
  std::map<std::string, Channel*> channels_;  // channel name -> Channel*

  ChannelManager(const ChannelManager& src);             // = delete
  ChannelManager& operator=(const ChannelManager& src);  // = delete
};

#endif
