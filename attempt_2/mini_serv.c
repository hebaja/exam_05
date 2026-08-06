#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct s_client
{
	int sock_fd;
	int id;
} t_client;

typedef struct s_server
{
	int sock_fd;
	t_client *clients;
	int clients_count;
	int capacity;
} t_server;

int extract_message(char **buf, char **msg)
{
	char *newbuf;
	int i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

void close_clients(t_server server)
{
	for (int i = 0; i < server.clients_count; i++)
		close(server.clients[i].sock_fd);
}

void broadcast(t_server server, char *msg, int bytes, t_client client)
{
	for (int i = 0; i < server.clients_count; i++)
	{
		if (server.clients[i].id == client.id)
			continue;
		send(server.clients[i].sock_fd, msg, bytes, 0);
	}
}

void broadcastServerMsg(t_server server, t_client client, int is_disconnect)
{
	char msg[512];
	int bytes;

	if (is_disconnect)
		bytes = sprintf(msg, "server: client %d just left\n", client.id);
	else
		bytes = sprintf(msg, "server: client %d just arrived\n", client.id);
	broadcast(server, msg, bytes, client);
}

int br_count(char *str)
{
	int count = 0;

	for (int i = 0; str[i] != '\0'; i++)
	{
		if (str[i] == '\n')
			count++;
	}
	return (count);
}

void broadcastMsg(t_server server, char *buffer, t_client client)
{
	char *extract_msg = NULL;
	char msg[512];
	int bytes;
	int len = br_count(buffer);

	for (int i = 0; i < len; i++)
	{
		char *to_free = buffer;
		extract_message(&buffer, &extract_msg);
		bytes = sprintf(msg, "client %d: %s", client.id, extract_msg);
		broadcast(server, msg, bytes, client);
		if (i > 0)
			free(to_free);
	}
	if (len > 0)
		free(buffer);
}

int main(int argc, char **argv)
{
	int sockfd, connfd, len, port;
	struct sockaddr_in servaddr, cli;
	t_server server;

	if (argc < 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}
	port = atoi(argv[1]);

	// socket create and verification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		write(2, "Fatal error\n", 12);
		exit(1);
	}
	bzero(&servaddr, sizeof(servaddr));

	int opt = 1;
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); // 127.0.0.1
	servaddr.sin_port = htons(port);
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	// Binding newly created socket to given IP and verification
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
	{
		write(2, "Fatal error\n", 12);
		exit(1);
	}
	if (listen(sockfd, 10) != 0)
	{
		write(2, "Fatal error\n", 12);
		exit(1);
	}

	bzero(&server, sizeof(t_server));
	server.sock_fd = sockfd;
	server.clients = calloc(1, sizeof(t_client));

	int running = 1;

	while (running)
	{
		fd_set readfds;
		int max_fd;

		FD_ZERO(&readfds);
		FD_SET(server.sock_fd, &readfds);
		max_fd = server.sock_fd;

		for (int i = 0; i < server.clients_count; i++)
		{
			FD_SET(server.clients[i].sock_fd, &readfds);
			if (server.clients[i].sock_fd > max_fd)
				max_fd = server.clients[i].sock_fd;
		}
		if (select(max_fd + 1, &readfds, NULL, NULL, 0) <= 0)
		{
			write(2, "Fatal error\n", 12);
			close(server.sock_fd);
			close_clients(server);
			free(server.clients);
			exit(1);
		}

		if (FD_ISSET(server.sock_fd, &readfds))
		{
			len = sizeof(cli);
			connfd = accept(sockfd, (struct sockaddr *)&cli, (socklen_t *)&len);
			if (connfd < 0)
			{
				write(2, "Fatal error\n", 12);
				close(server.sock_fd);
				close_clients(server);
				free(server.clients);
				exit(1);
			}

			t_client client;

			client.sock_fd = connfd;
			client.id = server.clients_count ? server.clients[server.clients_count - 1].id + 1 : 0;

			server.capacity = server.capacity ? server.capacity * 2 : 4;
			server.clients = realloc(server.clients, server.capacity * sizeof(t_client));
			if (!server.clients)
			{
				write(2, "Fatal error\n", 12);
				close(server.sock_fd);
				close_clients(server);
				free(server.clients);
				exit(1);
			}

			server.clients[server.clients_count++] = client;
			broadcastServerMsg(server, client, 0);
		}
		for (int i = 0; i < server.clients_count; i++)
		{
			if (FD_ISSET(server.clients[i].sock_fd, &readfds))
			{
				char buffer[512];
				int bytes;

				bytes = recv(server.clients[i].sock_fd, &buffer, sizeof(buffer), 0);

				if (bytes <= 0)
				{
					broadcastServerMsg(server, server.clients[i], 1);
					for (int pos = 0; pos < server.clients_count; pos++)
					{
						if (pos <= i)
							continue;
						server.clients[pos - 1] = server.clients[pos];
					}
					server.clients_count--;
				}
				else
				{
					buffer[bytes] = '\0';

					// if (buffer[0] == '\n' && strlen(buffer) == 1)
					//	continue;
					if (strcmp(buffer, "EXIT\n") == 0)
						running = 0;

					broadcastMsg(server, buffer, server.clients[i]);
				}
			}
		}
	}

	close(server.sock_fd);
	close_clients(server);
	free(server.clients);

	return (0);
}
