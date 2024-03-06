# Text-based Communicator (Client-Server)

## Overview

This repository contains the source code and documentation for a text-based communicator built on client-server architecture. The communicator utilizes the SCTP (Stream Control Transmission Protocol) and UDP (User Datagram Protocol) protocols, including multicast functionality.

## Team Members

- Jakub Mitura
- Adam Cypliński
- Bartłomiej Gromek

## Project Details

### 1. Project Assumptions

The communicator supports up to 10 clients connecting to the server. Clients can send messages, which the server relays to all other clients, excluding the sender. The SCTP protocol is used for message transmission. New client connections trigger a multicast message sent to all connected clients. Due to challenges with UDP multicast, SCTP is chosen for broadcasting messages to all clients.

To compile the project, include the following libraries:
```bash
gcc -o server server.c -lsctp -lpthread
gcc -o client client.c -lsctp -lpthread
```

### 2. Operation Principle

#### 2.1.1. Client Program

- Initializes SCTP and UDP sockets.
- Configures SCTP socket options with a custom IP address and port 5555.
- Joins a multicast group with the address 255.1.1.1 and port 2222.
- Connects to the server using IPv6 and provides a user name for identification.
- Creates two threads for handling SCTP and UDP message reception.
- Infinite loop allows users to input and send messages to the server.

#### 2.1.2. Server Program

- Uses SCTP (listen_sock, connect_sock) and UDP (sd) sockets.
- Utilizes daemonize() function to run the server as a background process.
- Accepts client connections and creates threads to handle clients.
- Receives user names, updates the list of connected users, and sends multicast messages about new client connections.
- Infinite loop for receiving messages using the sctp_recvmsg() function.

### 3. Tests

- a) Attempt to connect without entering the server's IP.
- b) Client connection to the server (client enters a name).
- c) Sending messages by the server.

### 4. Encountered Issues and Conclusions

- Issue: Client does not receive UDP multicast messages.
- Conclusion: The project does not fully support multicast due to client-side issues. Multicast works when the client is connected to the same IP as the server, but not from a different IP.

## Sources

- [RFC 4960 - SCTP](https://datatracker.ietf.org/doc/html/rfc4960#page-6)
- Lectures on Computer Networking
- Linux system manual (`man`)

Feel free to explore the source code and documentation to understand the implementation and contribute to further improvements. If you encounter issues or have suggestions, please open an issue in the repository. We appreciate your interest and collaboration!
