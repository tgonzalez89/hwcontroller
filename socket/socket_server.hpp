#pragma once

#include <cstring>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "../common/queue.hpp"


class SocketServer {
public:
  Queue<std::string> write_queue;
  Queue<std::string> read_queue;

  SocketServer() {
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
      error_state = server_socket;
      std::cout << "ERROR: socket creation failed" << std::endl;
      return;
    }

    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    int error = bind(server_socket, (struct sockaddr*)&server_address, sizeof(struct sockaddr));
    if (error) {
      error_state = error;
      std::cout << "ERROR: socket bind failed" << std::endl;
      return;
    }

    error = listen(server_socket, 1);
    if (error) {
      error_state = error;
      std::cout << "ERROR: socket listen failed" << std::endl;
      return;
    }

    sockaddr_in client_address;
    socklen_t sin_size = sizeof(struct sockaddr_in);
    client_socket = accept(server_socket, (struct sockaddr*)&client_address, &sin_size);
    if (client_socket < 0) {
      std::cout << "ERROR: socket accept failed" << std::endl;
      error_state = client_socket;
      return;
    }
  }

  void start() {
    std::scoped_lock lock(m);
    if (run_state == false) {
      run_state = true;
      t = std::thread( [this] { run(); } );
    }
  }

  void stop() {
    set_run_state(false);
    if (t.joinable()) t.join();
  }

  void run() {
    if (error_state) {
      close(client_socket);
      close(server_socket);
      return;
    }

    set_run_state(true);
    while (get_run_state()) {
      memset(buffer, 0, buf_size);
      int error = recv(client_socket, buffer, buf_size, 0);
      if (error < 0) {
        std::cout << "WARNING: read failed" << std::endl;
      }
      else {
        //std::cout << "DEBUG: SocketServer Received: " << buffer << std::endl;
      }

      std::string buf_str(buffer);
      if (buf_str.length() == 0)
        set_run_state(false);
      std::istringstream iss(buf_str);
      std::vector<std::string> tokens{
        std::istream_iterator<std::string>{iss},
        std::istream_iterator<std::string>{}
      };

      std::string msg = "None 0 0 0";
      if (tokens.size() > 0) {
        if (tokens[0] == "WriteReg") {
          write_queue.put(buf_str);
        }
        else if (tokens[0] == "GetNext") {
          msg = read_queue.get();
        }
      }

      memset(buffer, 0, buf_size);
      strcpy(buffer, msg.c_str());
      error =  send(client_socket, buffer, msg.length(), 0);
      if (error < 0) {
        std::cout << "WARNING: write failed" << std::endl;
      }
      else {
        //std::cout << "DEBUG: SocketServer Sent: " << msg << std::endl;
      }
    }

    close(client_socket);
    close(server_socket);
  }

  bool get_run_state() {
    std::scoped_lock lock(m);
    return run_state;
  }

  void set_run_state(bool val) {
    std::scoped_lock lock(m);
    run_state = val;
  }

private:
  const uint16_t port = 50007;
  static constexpr size_t buf_size = 1024;
  char buffer[buf_size];
  int server_socket;
  int client_socket;
  int error_state = 0;
  std::mutex m;
  volatile bool run_state = false;
  std::thread t;
};
