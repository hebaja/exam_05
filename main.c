#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

void emit_fatal_error() {
  write(2, "Fatal error\n", 12);
  exit(1);
}

typedef struct s_serv {
  int sock_fd;
  int *clients;
  int clients_count;
  int capacity;
  struct sockaddr_in addr;
} t_serv;

void set_server(t_serv *server, int port) {
  int opt = 1;

  server->sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  server->addr.sin_family = AF_INET;
  server->addr.sin_port = htons(port);
  server->addr.sin_addr.s_addr = INADDR_ANY;
  memset(server->addr.sin_zero, 0, sizeof(server->addr.sin_zero));
  setsockopt(server->sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  server->clients = calloc(1, sizeof(int));
  if (!server->clients)
  {
    emit_fatal_error();
    exit(1);
  }
}

void grow_clients(t_serv *server) {
  if (server->clients_count == server->capacity) {
    server->capacity = server->capacity ? server->capacity * 2 : 4;
    server->clients = realloc(server->clients, server->capacity * sizeof(int));
    if (!server->clients)
      emit_fatal_error();
  }
}

void shrink_clients(t_serv *server, int i) {
  close(server->clients[i]);
  server->clients[i] = server->clients[--server->clients_count]; // swap-remove
  if (server->clients_count < server->capacity / 2 && server->capacity > 4)
    server->capacity = server->capacity / 2;
  // (realloc down is optional; swap-remove already shrinks the set)
}

void run_server(t_serv *server) {
  int running = 1;

  while (running) {
    fd_set readfds;
    int max_fd = server->sock_fd;

    FD_ZERO(&readfds);
    FD_SET(server->sock_fd, &readfds);

    for (int i = 0; i < server->clients_count; i++) {
      FD_SET(server->clients[i], &readfds);
      if (server->clients[i] > max_fd)
        max_fd = server->clients[i];
    }
    if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0)
      emit_fatal_error();

    if (FD_ISSET(server->sock_fd, &readfds)) {
      int client_fd = accept(server->sock_fd, NULL, NULL);
      grow_clients(server);
      server->clients[server->clients_count++] = client_fd;
      printf("client connected %d\n", client_fd);
    }
    for (int i = 0; i < server->clients_count; i++) {
      if (FD_ISSET(server->clients[i], &readfds)) {
        char buffer[512];
        int bytes = recv(server->clients[i], buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
          shrink_clients(server, i);
          i--;
        } else {
          buffer[bytes] = '\0';
          write(1, buffer, bytes);
          if (strcmp(buffer, "EXIT\n") == 0) {
            printf("exting...\n");
            running = 0;
          }
        }
      }
    }
  }
}

int main(int argc, char **argv) 
{
  int     port;
  t_serv  server = {0};

  if (argc < 2)
  {
    write(2, "Wrong number of arguments\n", 26);
    exit(1);
  }

  port = atoi(argv[1]);
  set_server(&server, port);

  if (bind(server.sock_fd, (struct sockaddr *)&server.addr,
           sizeof(server.addr)) < 0)
    emit_fatal_error();
  if (listen(server.sock_fd, 10) < 0)
    emit_fatal_error();

  run_server(&server);

  free(server.clients);
  return (0);
}
