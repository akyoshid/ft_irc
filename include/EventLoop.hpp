#ifndef SRC_SERVER_EVENTLOOP_HPP_
#define SRC_SERVER_EVENTLOOP_HPP_

#include <sys/epoll.h>

#include <stdexcept>

#define INVALID_FD -1

// EventLoop: Wrapper class for epoll operations
// Manages the lifetime of an epoll instance and provides
// methods to add, modify, remove file descriptors, and wait for events.
class EventLoop {
 public:
  EventLoop();
  ~EventLoop();

  // Create an epoll instance
  void create();

  // Add a file descriptor to epoll
  // events: Events to monitor (EPOLLIN, EPOLLOUT, etc.)
  // EPOLLET (edge-triggered) is automatically added internally
  void addFd(int fd, uint32_t events) const;

  // Modify the events being monitored by epoll
  // events: New events to monitor
  // EPOLLET (edge-triggered) is automatically added internally
  void modifyFd(int fd, uint32_t events) const;

  // Remove a file descriptor from epoll
  void removeFd(int fd) const;

  // Wait for events
  // events: Array to store events
  // maxEvents: Maximum size of the array
  // timeout: Timeout in milliseconds, -1 for infinite wait
  // Returns: Number of events occurred, -1 on error
  int wait(struct epoll_event* events, int maxEvents, int timeout) const;

 private:
  int epollFd_;

  EventLoop(const EventLoop& src);             // = delete
  EventLoop& operator=(const EventLoop& src);  // = delete
};

#endif
