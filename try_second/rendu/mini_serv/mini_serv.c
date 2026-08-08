#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
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

void	emit_error(int sockfd, t_client *clients, int count)
{
	write(2, "Fatal error\n", 12);
	if (sockfd > 0)
		close(sockfd);
	for (int i = 0; i < count; i++) {
		close(clients[i].sockfd);
	}
	free(clients);
	exit(1);
}


void	broadcast(t_client *clients, char *msg, int bytes, int id, int count)
{
	for (int i = 0; i < count; i++) {
		if (clients[i].id == id)
			continue;
		send(clients[i].sockfd, msg, bytes, 0);
	}
}

void	broadcastServerMsg(t_client *clients, int id, int count, int is_disconnect)
{
	char	msg[1024];
	int	bytes;

	if (is_disconnect)
		bytes = sprintf(msg, "server: client %d just left\n", id);
	else
		bytes = sprintf(msg, "server: client %d just arrived\n", id);
	broadcast(clients, msg, bytes, id, count);
}

void	broadcastMsg(t_client *clients, char *buf, int id, int count)
{
	char	*extract;
	int	bytes;

	while (extract_message(&buf, &extract) == 1)
	{
		char	msg[1024];

		bytes = sprintf(msg, "client %d: %s", id, extract);
		broadcast(clients, msg, bytes, id, count);
		//free(extract);
	}
	//free(buf);
}

int main(int argc, char **argv) {
	int sockfd, connfd, count, capacity, max_fd, len, port, running;
	struct sockaddr_in servaddr, cli;
	t_client *clients;

	if (argc < 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}
	port = atoi(argv[1]);
	sockfd = 0;
	count = 0;
	capacity = 0;
	max_fd = 0;
	running = 1;
	clients = calloc(1, sizeof(t_client));

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1)
		emit_error(sockfd, clients, count);
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(port);

	int	opt = 1;
	setsockopt(sockfd, SOL_SOCKET , SO_REUSEADDR, (socklen_t *)&opt, sizeof(opt));

	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		emit_error(sockfd, clients, count);
	if (listen(sockfd, 10) != 0) 
		emit_error(sockfd, clients, count);
	
	while(running)
	{
		fd_set readfds;
		max_fd = sockfd;

		FD_ZERO(&readfds);
		FD_SET(sockfd, &readfds);

		for (int i = 0; i < count; i++) {
			FD_SET(clients[i].sockfd, &readfds);
			if (clients[i].sockfd > max_fd)
				max_fd = clients[i].sockfd;
		}

		select(max_fd + 1, &readfds, NULL, NULL, 0);

		if (FD_ISSET(sockfd, &readfds))
		{
			len = sizeof(cli);
			connfd = accept(sockfd, (struct sockaddr *)&cli, (socklen_t *)&len);
			if (connfd < 0)
				emit_error(sockfd, clients, count);
			t_client client;
			client.sockfd = connfd;
			client.id = count ? clients[count - 1].id + 1 : 0;
			capacity = capacity ? capacity * 2 : 4;
			clients = realloc(clients, sizeof(t_client) * capacity);
			clients[count++] = client;
			broadcastServerMsg(clients, client.id, count, 0);
		}
		for (int i = 0; i < count; i++) {
			if (FD_ISSET(clients[i].sockfd, &readfds))
			{
				char	*buf;
				int	bytes;

				buf = calloc(1024, sizeof(char));
				bytes = recv(clients[i].sockfd, buf, sizeof(buf), 0);
				if (bytes <= 0)
				{
					close(clients[i].sockfd);
					broadcastServerMsg(clients, clients[i].id, count, 1);
					for (int pos = 0; pos < count; pos++) {
						if (pos <= i)
							continue;
						clients[pos - 1] = clients[pos];
					}
					count--;
				}
				else
				{
					//if (bytes > 1023)
					//	emit_error(sockfd, clients, count);
					buf[bytes] = '\0';
					//if (strcmp(buf, "EXIT\n") == 0)
					//	running = 0;

					broadcastMsg(clients, buf, clients[i].id, count);
				}
				free(buf);
			}
		}
	}
	close(sockfd);
	for (int i = 0; i < count; i++) {
		close(clients[i].sockfd);
	}
	free(clients);
}
