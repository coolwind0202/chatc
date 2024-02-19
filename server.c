#include <sys/socket.h>
#include <sys/select.h>
#include <stdbool.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <stdio.h>
#include <fcntl.h>
#include <arpa/inet.h>

#define PORT 3000
#define BUF_SIZE 255

// Create server socket, and bind it to PORT
int setup_server_socket(uint16_t port)
{
  struct sockaddr_in name;

  // Create Server Socket
  int server_socket = socket(PF_INET, SOCK_STREAM, 0);
  if (server_socket < 0)
  {
    perror("Failed to create server socket");
    exit(EXIT_FAILURE);
  }

  // Bind the socket to PORT
  name.sin_addr.s_addr = htonl(INADDR_ANY);
  name.sin_port = htons(port);
  name.sin_family = AF_INET;

  if (bind(server_socket, (struct sockaddr *)&name, sizeof(name)) < 0)
  {
    perror("Failed to bind server socket to specified port");
    exit(EXIT_FAILURE);
  }

  printf("Server is waiting on %s:%d\n", "localhost", port);
  return server_socket;
}

int main(void)
{
  fd_set all_read_sockets, all_write_sockets, ready_read_sockets, ready_write_sockets;
  FD_ZERO(&all_read_sockets);
  FD_ZERO(&all_write_sockets);

  int server_socket = setup_server_socket(PORT);
  // The server socket is read only.
  FD_SET(server_socket, &all_read_sockets);

  if (listen(server_socket, 1) < 0)
  {
    perror("Failed to listen");
    exit(EXIT_FAILURE);
  }

  char buf[BUF_SIZE + 1] = {};

  while (true)
  {
    ready_read_sockets = all_read_sockets;
    // select() will block until some socket become ready.
    if (select(FD_SETSIZE, &ready_read_sockets, NULL, NULL, NULL) < 0)
    {
      perror("Failed to wait for ready of socket");
      exit(EXIT_FAILURE);
    }

    if (FD_ISSET(server_socket, &ready_read_sockets))
    {
      // Request to server socket. We will create new client socket.
      struct sockaddr_in client_name;
      socklen_t client_name_len;
      int client_socket = accept(server_socket, (struct sockaddr *)&client_name, &client_name_len);
      if (client_socket < 0)
      {
        perror("Failed to create client socket");
        exit(EXIT_FAILURE);
      }

      FD_SET(client_socket, &all_read_sockets);
      FD_SET(client_socket, &all_write_sockets);

      char addr_string[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &(client_name.sin_addr), addr_string, INET_ADDRSTRLEN);

      printf("New connection from %s:%d [fd:%d]\n", addr_string, ntohs(client_name.sin_port), client_socket);
      continue;
    }

    for (int i = 0; i < FD_SETSIZE; i++)
    {
      if (FD_ISSET(i, &ready_read_sockets))
      {
        // This fd is ready to read.

        // Client socket is readable.
        ssize_t recv_len = recv(i, buf, BUF_SIZE, 0);
        if (recv_len < 0)
        {
          perror("Failed to receive message");
          exit(EXIT_FAILURE);
        }

        if (recv_len == 0)
        {
          FD_CLR(i, &all_read_sockets);
          continue;
        }

        buf[recv_len] = '\0';

        printf("Message Content is %s\n", buf);
        for (int j = 0; j < FD_SETSIZE; j++)
        {
          // Don't send message to itself.
          if (j == i)
            continue;

          if (!FD_ISSET(j, &all_write_sockets))
          {
            continue;
          }

          if (send(j, buf, recv_len, 0) < 0)
          {
            perror("Failed to send message");
            fprintf(stderr, "File descriptor: %d\n", j);

            exit(EXIT_FAILURE);
          }
          else
          {
            printf("Sent to %d\n", j);
          }
        }
      }
    }
  }

  return 0;
}