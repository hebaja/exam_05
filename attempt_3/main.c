#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>

typedef struct s_client
{
	int	id;
	int	sockfd;
}	t_client;

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

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

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

void	emit_fatal_error(int sockfd, t_client *clients, int clients_count)
{
	write(2, "Fatal error\n", 12);
	if (sockfd >= 0)
		close(sockfd);
	for (int i = 0; i < clients_count; i++)
		close(clients[i].sockfd);
	free(clients);
	exit(1);
}

void 	broadcast(t_client *clients, int clients_count, char *msg, int id, int bytes)
{
	for (int i = 0; i < clients_count; i++)
	{
		if (clients[i].id == id)
			continue;
		send(clients[i].sockfd, msg, bytes, 0);
	}
}

void	broadcastServerMsg(t_client *clients, int clients_count, int id, int is_disconnect)
{
	char	msg[512];
	int	bytes;

	if (is_disconnect)
		bytes = sprintf(msg, "server: client %d just left\n", id);
	else
		bytes = sprintf(msg, "server: client %d just arrived\n", id);

	broadcast(clients, clients_count, msg, id, bytes);
}

void	broadcastMsg(t_client *clients, int clients_count, char *buf, int pos)
{
	char	*msg = NULL;
	char	*buffer;
	int	bytes;

	buffer = calloc(strlen(buf) + 1, sizeof(char));
	strcpy(buffer, buf);
	while (extract_message(&buffer, &msg) == 1)
	{
		char format[512];
		bytes = sprintf(format, "client %d: %s", clients[pos].id, msg);
		broadcast(clients, clients_count, format, clients[pos].id, bytes);
		free(msg);
	}
	free(buffer);
}

int main(int argc, char **argv) {
	int sockfd, connfd, len, capacity, clients_count, running, max_fd, port;
	struct sockaddr_in servaddr, cli;
	t_client *clients;

	if (argc < 2)
	{
		write(1, "Wrong number of arguments\n", 26);
		exit(1);
	}
	port = atoi(argv[1]);
	capacity = 0;
	clients_count = 0;
	running = 1;
	sockfd = 0;
	clients = calloc(1, sizeof(t_client));
	if (!clients)
		emit_fatal_error(sockfd, clients, clients_count);
	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1)
		emit_fatal_error(sockfd, clients, clients_count);
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(port);
       
	int opt = 1;
	//setsockopt(sockfd, , int option_name, const void *option_value, socklen_t option_len);
	//fcntl(sockfd, F_SETFL, O_NONBLOCK);
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		emit_fatal_error(sockfd, clients, clients_count);
	if (listen(sockfd, 10) != 0)
		emit_fatal_error(sockfd, clients, clients_count);


	while (running)
	{
		fd_set readfds;
		max_fd = sockfd;

		FD_ZERO(&readfds);
		FD_SET(sockfd, &readfds);

		for (int i = 0; i < clients_count; i++) {
			FD_SET(clients[i].sockfd, &readfds);
			if (clients[i].sockfd > max_fd)
				max_fd = clients[i].sockfd;
		}

		if (select(max_fd + 1, &readfds, NULL, NULL, 0) < 0)
			emit_fatal_error(sockfd, clients, clients_count);

		if (FD_ISSET(sockfd, &readfds))
		{
			len = sizeof(cli);
			connfd = accept(sockfd, (struct sockaddr *)&cli, (socklen_t *)&len);
			if (connfd < 0)
			       emit_fatal_error(sockfd, clients, clients_count);

			t_client client;
			client.sockfd = connfd;
			client.id = clients_count ? clients[clients_count - 1].id + 1 : 0;

			capacity = capacity ? capacity * 2 : 4;
			clients = realloc(clients, capacity * sizeof(t_client));
			clients[clients_count++] = client;
			broadcastServerMsg(clients, clients_count, client.id, 0);
		}
		for (int i = 0; i < clients_count; i++) {
			if (FD_ISSET(clients[i].sockfd, &readfds))
			{
				char	buf[512];
				int	bytes;

				bytes = recv(clients[i].sockfd, &buf, sizeof(buf), 0);
				if (bytes <= 0)
				{
					broadcastServerMsg(clients, clients_count, clients[i].id, 1);
					close(clients[i].sockfd);
					for (int pos = 0; pos < clients_count; pos++) {
						if (pos <= i)
							continue;
						clients[pos - 1] = clients[pos];
					}
					clients_count--;
				}
				else
				{
					buf[bytes] = '\0';

					if (strcmp(buf, "EXIT\n") == 0)
						running = 0;

					broadcastMsg(clients, clients_count, buf, i);
				}
			}
		}
	}
	close(sockfd);
	for (int i = 0; i < clients_count; i++)
		close(clients[i].sockfd);
	free(clients);
}
