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

void emit_fatal_error()
{
	write(2, "Fatal error\n", 12);
	exit(1);
}

typedef struct s_cli
{
	int sock_fd;
	int id;
} t_cli;

typedef struct s_serv
{
	int sock_fd;
	// int *clients;
	t_cli *clis;
	int clients_count;
	int capacity;
	struct sockaddr_in addr;
} t_serv;

void set_server(t_serv *server, int port)
{
	int opt = 1;

	server->sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	server->addr.sin_family = AF_INET;
	server->addr.sin_port = htons(port);
	server->addr.sin_addr.s_addr = INADDR_ANY;
	memset(server->addr.sin_zero, 0, sizeof(server->addr.sin_zero));
	setsockopt(server->sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	server->clis = calloc(1, sizeof(t_cli));
	if (!server->clis)
	{
		emit_fatal_error();
		exit(1);
	}
}

void grow_clients(t_serv *server)
{
	if (server->clients_count == server->capacity)
	{
		server->capacity = server->capacity ? server->capacity * 2 : 4;
		server->clis = realloc(server->clis, server->capacity * sizeof(t_cli));
		if (!server->clis)
		{
			emit_fatal_error();
			exit(1);
		}
	}
}

void shrink_clients(t_serv *server, int pos)
{
	close(server->clis[pos].sock_fd);
	
	for (int i = 0; i < server->clients_count; i++)
	{
		if (i <= pos)
			continue;
		server->clis[i - 1] = server->clis[i];
	}
	server->clients_count--;
}

void broadcast(t_serv *server, t_cli client, char *str, int bytes)
{
	for (int i = 0; i < server->clients_count; i++)
	{
		if (server->clis[i].id == client.id)
			continue;
		send(server->clis[i].sock_fd, str, bytes, 0);
	}
}

void broadcastMessage(t_serv *server, char *buffer, t_cli client)
{
	char str[512];

	int new_bytes = sprintf(str, "client %d: %s", client.id, buffer);

	broadcast(server, client, str, new_bytes);
}

void broadcastServerMessage(t_serv *server, t_cli client, int is_cli_disconnect)
{
	int bytes;
	char str[512];

	if (is_cli_disconnect)
		bytes = sprintf(str, "server: client %d just left\n", client.id);
	else
		bytes = sprintf(str, "server: client %d just arrived\n", client.id);

	broadcast(server, client, str, bytes);
}

void run_server(t_serv *server)
{
	int running = 1;

	while (running)
	{
		fd_set readfds;
		int max_fd = server->sock_fd;

		FD_ZERO(&readfds);
		FD_SET(server->sock_fd, &readfds);

		for (int i = 0; i < server->clients_count; i++)
		{
			FD_SET(server->clis[i].sock_fd, &readfds);
			if (server->clis[i].sock_fd > max_fd)
				max_fd = server->clis[i].sock_fd;
		}
		if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0)
			emit_fatal_error();

		if (FD_ISSET(server->sock_fd, &readfds))
		{
			int client_fd = accept(server->sock_fd, NULL, NULL);

			t_cli client;

			client.sock_fd = client_fd;
			client.id = server->clients_count ? server->clis[server->clients_count - 1].id + 1 : 0;

			grow_clients(server);
			server->clis[server->clients_count++] = client;
			broadcastServerMessage(server, client, 0);
		}
		for (int i = 0; i < server->clients_count; i++)
		{
			if (FD_ISSET(server->clis[i].sock_fd, &readfds))
			{
				char buffer[512];
				int bytes = recv(server->clis[i].sock_fd, buffer, sizeof(buffer), 0);
				if (bytes <= 0)
				{
					broadcastServerMessage(server, server->clis[i], 1);
					shrink_clients(server, i);
					i--;
				}
				else
				{
					buffer[bytes] = '\0';
					broadcastMessage(server, buffer, server->clis[i]);
					if (strcmp(buffer, "EXIT\n") == 0)
					{
						running = 0;
						for (int i = 0; i < server->clients_count; i++)
							close(server->clis[i].sock_fd);
					}
				}

				for (int i = 0; i < server->clients_count; i++) {
					printf("id -> %d: fd -> %d\n", server->clis[i].id, server->clis[i].sock_fd);
				}
				printf("\n");


			}
		}
	}
}

int main(int argc, char **argv)
{
	int port;
	t_serv server = {0};

	if (argc < 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}

	port = atoi(argv[1]);
	set_server(&server, port);

	if (bind(server.sock_fd, (struct sockaddr *)&server.addr, sizeof(server.addr)) < 0)
		emit_fatal_error();
	if (listen(server.sock_fd, 10) < 0)
		emit_fatal_error();

	run_server(&server);

	close(server.sock_fd);

	free(server.clis);
	return (0);
}
