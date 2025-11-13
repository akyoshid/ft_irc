/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   BotClient.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akyoshid <akyoshid@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/13 00:35:00 by akyoshid          #+#    #+#             */
/*   Updated: 2025/11/13 03:53:05 by akyoshid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "BotClient.hpp"

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <stdexcept>

#include "utils.hpp"

namespace {
const int kMaxEvents = 10;
const int kReadBufferSize = 4096;

bool startsWith(const std::string& text, const std::string& prefix) {
  return text.compare(0, prefix.size(), prefix) == 0;
}

enum RpsHand { RPS_ROCK = 0, RPS_PAPER, RPS_SCISSORS };

bool parseRpsHand(const std::string& token, RpsHand* out) {
  if (token == "rock" || token == "r") {
    *out = RPS_ROCK;
    return true;
  }
  if (token == "paper" || token == "p") {
    *out = RPS_PAPER;
    return true;
  }
  if (token == "scissors" || token == "s") {
    *out = RPS_SCISSORS;
    return true;
  }
  return false;
}

std::string rpsHandToString(RpsHand hand) {
  switch (hand) {
    case RPS_ROCK:
      return "rock";
    case RPS_PAPER:
      return "paper";
    case RPS_SCISSORS:
      return "scissors";
  }
  return "unknown";
}

RpsHand randomRpsHand() {
  static bool seeded = false;
  if (!seeded) {
    std::srand(static_cast<unsigned int>(std::time(NULL)));
    seeded = true;
  }
  return static_cast<RpsHand>(std::rand() % 3);
}

std::string rpsResultMessage(RpsHand userHand, RpsHand botHand) {
  if (userHand == botHand) {
    return "It's a tie!";
  }
  if ((userHand == RPS_ROCK && botHand == RPS_SCISSORS) ||
      (userHand == RPS_PAPER && botHand == RPS_ROCK) ||
      (userHand == RPS_SCISSORS && botHand == RPS_PAPER)) {
    return "You win!";
  }
  return "You lose";
}

}  // namespace

BotClient::BotClient(const std::string& host, const std::string& port,
                     const std::string& password, const std::string& nickname,
                     const std::string& channel)
    : host_(host),
      port_(port),
      password_(password),
      nickname_(nickname),
      channel_(channel),
      socketFd_(INVALID_FD),
      running_(true),
      connectionVerified_(false),
      registered_(false),
      joined_(false) {}

BotClient::~BotClient() { closeSocket(); }

void BotClient::run() {
  connectToServer();
  setupEventLoop();
  sendInitialMessages();
  handleEvents();
}

void BotClient::connectToServer() {
  struct addrinfo hints;
  struct addrinfo* result = NULL;

  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  int status = getaddrinfo(host_.c_str(), port_.c_str(), &hints, &result);
  if (status != 0) {
    throw std::runtime_error(std::string("getaddrinfo: ") +
                             gai_strerror(status));
  }

  int errsv = 0;
  try {
    for (struct addrinfo* rp = result; rp != NULL; rp = rp->ai_next) {
      int fd = -1;
      try {
        fd = createNonBlockingSocket();
        int connectResult = connect(fd, rp->ai_addr, rp->ai_addrlen);
        if (connectResult == 0 || errno == EINPROGRESS) {
          socketFd_ = fd;
          break;
        }
        errsv = errno;
        close(fd);
      } catch (...) {
        if (fd != -1) close(fd);
        throw;
      }
    }
    if (socketFd_ == INVALID_FD) {
      freeaddrinfo(result);
      throw std::runtime_error(
          std::string("Failed to connect to any address: ") +
          createErrorMessage("connect", errsv));
    }
  } catch (...) {
    freeaddrinfo(result);
    throw;
  }

  freeaddrinfo(result);
  log(LOG_LEVEL_INFO, LOG_CATEGORY_NETWORK, "Connecting to IRC server...");
}

int BotClient::createNonBlockingSocket() const {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) throw std::runtime_error(createErrorMessage("socket", errno));
  if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
    throw std::runtime_error(createErrorMessage("fcntl(F_SETFL)", errno));
  return fd;
}

void BotClient::setupEventLoop() {
  eventLoop_.create();
  eventLoop_.addFd(socketFd_, EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLERR);
}

void BotClient::handleEvents() {
  struct epoll_event events[kMaxEvents];

  while (running_) {
    int n = eventLoop_.wait(events, kMaxEvents, -1);
    if (n == -1) {
      if (errno == EINTR) continue;
      throw std::runtime_error(createErrorMessage("epoll_wait", errno));
    }
    for (int i = 0; i < n; ++i) {
      uint32_t ev = events[i].events;
      if (ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
        running_ = false;
        log(LOG_LEVEL_WARNING, LOG_CATEGORY_CONNECTION,
            "Connection closed unexpectedly");
        break;
      }
      if (ev & EPOLLIN) {
        handleRead();
      }
      if (ev & EPOLLOUT) {
        handleWrite();
      }
    }
  }
}

void BotClient::handleRead() {
  char buffer[kReadBufferSize];
  while (true) {
    ssize_t bytes = recv(socketFd_, buffer, sizeof(buffer), 0);
    if (bytes > 0) {
      readBuffer_.append(buffer, bytes);
    } else if (bytes == 0) {
      running_ = false;
      return;
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK) break;
      throw std::runtime_error(createErrorMessage("recv", errno));
    }
  }

  std::string::size_type pos;
  while ((pos = readBuffer_.find("\r\n")) != std::string::npos) {
    std::string line = readBuffer_.substr(0, pos);
    readBuffer_.erase(0, pos + 2);
    processMessage(line);
  }
}

void BotClient::handleWrite() {
  if (!connectionVerified_) {
    int error = 0;
    socklen_t len = sizeof(error);
    if (getsockopt(socketFd_, SOL_SOCKET, SO_ERROR, &error, &len) < 0 ||
        error != 0) {
      throw std::runtime_error("Connection failed");
    }
    connectionVerified_ = true;
  }
  while (!writeBuffer_.empty()) {
    ssize_t bytes =
        send(socketFd_, writeBuffer_.c_str(), writeBuffer_.size(), 0);
    if (bytes > 0) {
      writeBuffer_.erase(0, bytes);
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK) break;
      throw std::runtime_error(createErrorMessage("send", errno));
    }
  }
  if (writeBuffer_.empty()) {
    eventLoop_.modifyFd(socketFd_, EPOLLIN | EPOLLRDHUP | EPOLLERR);
  }
}

void BotClient::processMessage(const std::string& line) {
  log(LOG_LEVEL_DEBUG, LOG_CATEGORY_NETWORK, "<< " + line);

  std::string prefix;
  std::string command;
  std::vector<std::string> params;
  std::string trailing;

  std::string::size_type pos = 0;
  std::string::size_type end = line.size();

  if (pos < end && line[pos] == ':') {
    std::string::size_type space = line.find(' ', pos);
    if (space == std::string::npos) return;
    prefix = line.substr(pos + 1, space - pos - 1);
    pos = space + 1;
  }

  std::string::size_type space = line.find(' ', pos);
  if (space == std::string::npos) {
    command = line.substr(pos);
  } else {
    command = line.substr(pos, space - pos);
    pos = space + 1;
  }

  while (pos < end) {
    if (line[pos] == ':') {
      trailing = line.substr(pos + 1);
      break;
    }
    std::string::size_type nextSpace = line.find(' ', pos);
    if (nextSpace == std::string::npos) {
      params.push_back(line.substr(pos));
      break;
    }
    params.push_back(line.substr(pos, nextSpace - pos));
    pos = nextSpace + 1;
  }

  processCommand(prefix, command, params, trailing);
}

void BotClient::processCommand(const std::string& prefix,
                               const std::string& command,
                               const std::vector<std::string>& params,
                               const std::string& trailing) {
  if (command == "PING") {
    enqueueMessage("PONG :" + trailing);
    return;
  }
  if (command == "001") {
    registered_ = true;
    joinChannelIfNeeded();
    return;
  }
  if (command == "376" || command == "422") {
    joinChannelIfNeeded();
    return;
  }
  if (command == "PRIVMSG") {
    handlePrivMsg(prefix, params, trailing);
  }
}

void BotClient::joinChannelIfNeeded() {
  if (!joined_ && registered_) {
    enqueueMessage("JOIN " + channel_);
    joined_ = true;
    log(LOG_LEVEL_INFO, LOG_CATEGORY_CONNECTION, "Joined channel " + channel_);
  }
}

void BotClient::handlePrivMsg(const std::string& prefix,
                              const std::vector<std::string>& params,
                              const std::string& trailing) {
  if (params.empty()) return;
  std::string target = params[0];
  std::string sender = extractNickname(prefix);

  std::string messageText = trailing;
  if (messageText.empty() && params.size() > 1) {
    messageText = params[params.size() - 1];
  }

  if (messageText.empty()) return;

  log(LOG_LEVEL_DEBUG, LOG_CATEGORY_COMMAND,
      "Received command message: '" + messageText + "'");

  if (!startsWith(messageText, "!")) return;

  respondToUser(target, messageText, sender);
}

void BotClient::respondToUser(const std::string& target,
                              const std::string& command,
                              const std::string& senderNick) {
  std::string responseTarget = target;
  if (normalizeNickname(target) == normalizeNickname(nickname_)) {
    responseTarget = senderNick;
  }

  std::string lower = command;
  for (size_t i = 0; i < lower.size(); ++i) {
    lower[i] =
        static_cast<char>(std::tolower(static_cast<unsigned char>(lower[i])));
  }

  std::string message;
  if (lower == "!help") {
    message = "Available commands: !help, !time, !ping, !about, !rps <hand>";
  } else if (lower == "!time") {
    message = "Current time: " + currentTimeString();
  } else if (lower == "!ping") {
    message = "Pong!";
  } else if (lower == "!about") {
    message = "I am an IRC bot built for ft_irc";
  } else if (startsWith(lower, "!rps")) {
    const std::string::size_type commandLength = 4;  // strlen("!rps")
    if (lower.size() == commandLength) {
      message = "Usage: !rps <rock|paper|scissors>";
    } else if (!std::isspace(
                   static_cast<unsigned char>(lower[commandLength]))) {
      return;
    } else {
      std::string::size_type pos = commandLength;
      while (pos < lower.size() &&
             std::isspace(static_cast<unsigned char>(lower[pos]))) {
        ++pos;
      }
      if (pos == lower.size()) {
        message = "Usage: !rps <rock|paper|scissors>";
      } else {
        std::string::size_type end = pos;
        while (end < lower.size() &&
               !std::isspace(static_cast<unsigned char>(lower[end]))) {
          ++end;
        }
        std::string token = lower.substr(pos, end - pos);
        RpsHand userHand;
        if (!parseRpsHand(token, &userHand)) {
          message = "Invalid hand. Use rock(r), paper(p), or scissors(s).";
        } else {
          RpsHand botHand = randomRpsHand();
          std::string result = rpsResultMessage(userHand, botHand);
          message = "You chose " + rpsHandToString(userHand) + ", I chose " +
                    rpsHandToString(botHand) + ". " + result;
        }
      }
    }
  } else {
    return;
  }

  enqueueMessage("PRIVMSG " + responseTarget + " :" + message);
  log(LOG_LEVEL_INFO, LOG_CATEGORY_COMMAND,
      "Responded to command '" + command + "'");
}

void BotClient::enqueueMessage(const std::string& message) {
  if (!startsWith(message, "PASS ")) {
    log(LOG_LEVEL_DEBUG, LOG_CATEGORY_NETWORK, ">> " + message);
  } else {
    log(LOG_LEVEL_DEBUG, LOG_CATEGORY_NETWORK, ">> PASS ***");
  }
  writeBuffer_ += message + "\r\n";
  eventLoop_.modifyFd(socketFd_, EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLERR);
  handleWrite();
}

std::string BotClient::extractNickname(const std::string& prefix) const {
  std::string::size_type pos = prefix.find('!');
  if (pos == std::string::npos) return prefix;
  return prefix.substr(0, pos);
}

std::string BotClient::currentTimeString() const {
  std::time_t now = std::time(NULL);
  std::tm tm = *std::localtime(&now);
  char buffer[64];
  if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm) == 0)
    return "Unknown";
  return buffer;
}

void BotClient::sendInitialMessages() {
  if (!password_.empty()) {
    enqueueMessage("PASS " + password_);
  }
  enqueueMessage("NICK " + nickname_);
  enqueueMessage("USER " + nickname_ + " 0 * :" + nickname_);
}

void BotClient::closeSocket() {
  if (socketFd_ != INVALID_FD) {
    close(socketFd_);
    socketFd_ = INVALID_FD;
  }
}
