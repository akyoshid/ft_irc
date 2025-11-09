#ifndef SRC_SERVER_CONNECTIONMANAGER_HPP_
#define SRC_SERVER_CONNECTIONMANAGER_HPP_

#include <map>
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

// ConnectionManager: Manages user connections and I/O operations
// Handles accepting new connections, disconnecting users,
// receiving and sending data
class ConnectionManager {
 public:
  ConnectionManager();
  ~ConnectionManager();

  // Accept a new connection from the server socket
  // Returns: Pointer to new User object, or NULL if no connection available
  // Throws: std::runtime_error on error
  User* acceptConnection(int serverFd);

  // Disconnect a user and clean up resources
  // Removes from map, closes socket, deletes User object
  void disconnect(int userFd);

  // Disconnect all users (used in destructor)
  void disconnectAll();

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

  // Get a user by file descriptor
  // Returns: Pointer to User, or NULL if not found
  User* getUser(int fd);

  // Get all users (for Server class access)
  const std::map<int, User*>& getUsers() const;

 private:
  std::map<int, User*> users_;

  ConnectionManager(const ConnectionManager& src);             // = delete
  ConnectionManager& operator=(const ConnectionManager& src);  // = delete
};

#endif
