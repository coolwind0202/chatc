#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>

#define PORT 3000
#define HOST "localhost"
#define BUF_SIZE 255

int connect_socket()
{
  int client_socket = socket(PF_INET, SOCK_STREAM, 0);
  struct sockaddr_in name;
  struct hostent *hostinfo;

  name.sin_family = AF_INET;
  name.sin_port = htons(PORT);

  hostinfo = gethostbyname(HOST);

  if (hostinfo == NULL)
  {
    fprintf(stderr, "Unknown host %s.\n", HOST);
    exit(EXIT_FAILURE);
  }

  name.sin_addr = *(struct in_addr *)hostinfo->h_addr_list[0];

  if (connect(client_socket, (struct sockaddr *)&name, sizeof(name)) < 0)
  {
    perror("Failed to connect");
    exit(EXIT_FAILURE);
  }

  return client_socket;
}

int main(void)
{
  int client_socket = connect_socket();

  fd_set all_read_fds, ready_read_fds;
  FD_ZERO(&all_read_fds);
  FD_ZERO(&ready_read_fds);
  FD_SET(STDIN_FILENO, &all_read_fds);
  FD_SET(client_socket, &all_read_fds);

  char buf[BUF_SIZE + 1] = {};

  while (true)
  {
    ready_read_fds = all_read_fds;
    if (select(FD_SETSIZE, &ready_read_fds, NULL, NULL, NULL) < 0)
    {
      perror("Failed to wait for ready of file descriptors");
      exit(EXIT_FAILURE);
    }

    if (FD_ISSET(STDIN_FILENO, &ready_read_fds))
    {
      // The user inputted some string.
      if (fgets(buf, BUF_SIZE + 1, stdin) == NULL)
      {
        perror("Failed to read standard input");
        exit(EXIT_FAILURE);
      }

      send(client_socket, buf, strlen(buf), 0);
    }

    if (FD_ISSET(client_socket, &ready_read_fds))
    {
      // Some messages were sent from client socket.
      ssize_t recv_len = recv(client_socket, buf, BUF_SIZE, 0);
      if (recv_len < 0)
      {
        perror("Failed to receive from server");
        exit(EXIT_FAILURE);
      }

      if (recv_len == 0)
      {
        fputs("Connection was end.\n", stderr);
        exit(EXIT_SUCCESS);
      }

      buf[recv_len] = '\0';
      write(STDOUT_FILENO, buf, recv_len);
    }
  }

  return 0;
}