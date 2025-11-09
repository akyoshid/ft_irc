#include "server/EventLoop.hpp"

#include <sys/epoll.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>

#include "utils.hpp"

EventLoop::EventLoop() : epollFd_(INVALID_FD) {}

EventLoop::~EventLoop() {
  if (epollFd_ != INVALID_FD) {
    close(epollFd_);
  }
}

void EventLoop::create() {
  // Create an epoll instance
  // The argument 0 means "no special flags"
  epollFd_ = epoll_create1(0);
  if (epollFd_ < 0) {
    int errsv = errno;
    throw std::runtime_error(createErrorMessage("epoll_create1", errsv));
  }
}

void EventLoop::addFd(int fd, uint32_t events) const {
  struct epoll_event ev;
  std::memset(&ev, 0, sizeof(ev));
  // Always use edge-triggered mode (EPOLLET)
  ev.events = events | EPOLLET;
  ev.data.fd = fd;

  if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
    int errsv = errno;
    throw std::runtime_error(createLog(LOG_LEVEL_ERROR, LOG_CATEGORY_SYSTEM,
                                       createErrorMessage("epoll_ctl", errsv)));
  }
}

void EventLoop::modifyFd(int fd, uint32_t events) const {
  struct epoll_event ev;
  std::memset(&ev, 0, sizeof(ev));
  // Always use edge-triggered mode (EPOLLET)
  ev.events = events | EPOLLET;
  ev.data.fd = fd;

  if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev) < 0) {
    int errsv = errno;
    throw std::runtime_error(createLog(LOG_LEVEL_ERROR, LOG_CATEGORY_SYSTEM,
                                       createErrorMessage("epoll_ctl", errsv)));
  }
}

void EventLoop::removeFd(int fd) const {
  // In Linux 2.6.9+, the event argument can be NULL for EPOLL_CTL_DEL
  if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, NULL) < 0) {
    int errsv = errno;
    throw std::runtime_error(createLog(LOG_LEVEL_ERROR, LOG_CATEGORY_SYSTEM,
                                       createErrorMessage("epoll_ctl", errsv)));
  }
}

int EventLoop::wait(struct epoll_event* events, int maxEvents, int timeout) const {
  return epoll_wait(epollFd_, events, maxEvents, timeout);
}
