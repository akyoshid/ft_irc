#ifndef SRC_SERVER_CONNECTIONMANAGER_HPP_
#define SRC_SERVER_CONNECTIONMANAGER_HPP_

#include <map>
#include <string>
#include <vector>

#include "Client.hpp"

#define BUFFER_SIZE 4096
#define MAX_BUFFER_SIZE 8192

// Result codes for receive operation
enum ReceiveResult {
  RECV_SUCCESS,  // Data received successfully
  RECV_CLOSED,   // Connection closed by client
  RECV_ERROR     // Error occurred during receive
};

// Result codes for send operation
enum SendResult {
  SEND_SUCCESS,   // Data sent (may have more to send)
  SEND_COMPLETE,  // All data sent, buffer empty
  SEND_ERROR      // Error occurred during send
};

// ConnectionManager: Manages client connections and I/O operations
// Handles accepting new connections, disconnecting clients,
// receiving and sending data
class ConnectionManager {
 public:
  ConnectionManager();
  ~ConnectionManager();

  // Accept a new connection from the server socket
  // Returns: Pointer to new Client object, or NULL if no connection available
  // Throws: std::runtime_error on error
  Client* acceptConnection(int serverFd);

  // Disconnect a client and clean up resources
  // Removes from map, closes socket, deletes Client object
  void disconnect(int clientFd);

  // Disconnect all clients (used in destructor)
  void disconnectAll();

  // Receive data from a client
  // Reads data into read buffer, extracts complete messages
  // messages: Vector to store extracted messages (messages ending with \r\n)
  // Returns: RECV_SUCCESS, RECV_CLOSED, or RECV_ERROR
  ReceiveResult receiveData(Client* client, std::vector<std::string>& messages);

  // Send data to a client
  // Sends data from write buffer
  // Returns: SEND_SUCCESS (more to send), SEND_COMPLETE (buffer empty), or
  // SEND_ERROR
  SendResult sendData(Client* client);

  // Get a client by file descriptor
  // Returns: Pointer to Client, or NULL if not found
  Client* getClient(int fd);

  // Get all clients (for Server class access)
  const std::map<int, Client*>& getClients() const;

 private:
  std::map<int, Client*> clients_;

  ConnectionManager(const ConnectionManager& src);             // = delete
  ConnectionManager& operator=(const ConnectionManager& src);  // = delete
};

#endif
