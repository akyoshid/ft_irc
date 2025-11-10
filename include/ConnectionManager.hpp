#ifndef INCLUDE_CONNECTIONMANAGER_HPP_
#define INCLUDE_CONNECTIONMANAGER_HPP_

#include <string>
#include <vector>

#include "User.hpp"

#define BUFFER_SIZE 4096
#define MAX_BUFFER_SIZE 8192

// Result codes for receive operation
enum ReceiveResult {
  RECV_SUCCESS,  // Data received successfully
  RECV_CLOSED,   // Connection closed by user
  RECV_ERROR     // Error occurred during receive
};

// Result codes for send operation
enum SendResult {
  SEND_SUCCESS,   // Data sent (may have more to send)
  SEND_COMPLETE,  // All data sent, buffer empty
  SEND_ERROR      // Error occurred during send
};

// ConnectionManager: Manages I/O operations for user connections
// Handles accepting new connections, receiving and sending data
// Note: User management is handled by UserManager
class ConnectionManager {
 public:
  ConnectionManager();
  ~ConnectionManager();

  // Accept a new connection from the server socket
  // Returns: Pointer to new User object, or NULL if no connection available
  // Throws: std::runtime_error on error
  // Note: Caller is responsible for adding user to UserManager
  User* acceptConnection(int serverFd);

  // Receive data from a user
  // Reads data into read buffer, extracts complete messages
  // messages: Vector to store extracted messages (messages ending with \r\n)
  // Returns: RECV_SUCCESS, RECV_CLOSED, or RECV_ERROR
  ReceiveResult receiveData(User* user, std::vector<std::string>& messages);

  // Send data to a user
  // Sends data from write buffer
  // Returns: SEND_SUCCESS (more to send), SEND_COMPLETE (buffer empty), or
  // SEND_ERROR
  SendResult sendData(User* user);

 private:
  ConnectionManager(const ConnectionManager& src);             // = delete
  ConnectionManager& operator=(const ConnectionManager& src);  // = delete
};

#endif
