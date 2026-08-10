#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct s_cli
{
	int	id;
	int	sockfd;
	char	*buf;
}	t_cli;

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

void	emit_error(int sockfd, t_cli *clients, int count)
{
	if (sockfd > 0)
		close(sockfd);
	for (int i = 0; i < count; i++) {
		close(clients[i].sockfd);
		free(clients[i].buf);
	}
	free(clients);
	write(2, "Fatal error\n", 12);
	exit(1);
}
/*
"server: client %d just arrived\n"
"server: client %d just left\n"
"Wrong number of arguments"
"Fatal error"
*/

void	broadcast(t_cli *clients, char *msg, int count, ssize_t bytes, int id)
{
	for (int i = 0; i < count; i++) {
		if (clients[i].id == id)
			continue;
		send(clients[i].sockfd, msg, bytes, 0);
	}
}

void	broadcastServerMsg(t_cli *clients, int count, int id, int is_disconn)
{
	char	msg[4096];
	ssize_t	bytes;

	if (is_disconn)
		bytes = sprintf(msg, "server: client %d just left\n", id);
	else
		bytes = sprintf(msg, "server: client %d just arrived\n", id);
	broadcast(clients, msg, count, bytes, id);
}

void	broadcastMsg(int sockfd, t_cli *clients, int count, int pos)
{
	char	*msg;
	char	*extract;
	ssize_t	bytes;

	while (extract_message(&clients[pos].buf, &extract) == 1)
	{
		msg = calloc(strlen(extract) + 32, sizeof(char));
		if (msg == NULL)
		{
			free(extract);
			emit_error(sockfd, clients, count);
		}
		bytes = sprintf(msg, "client %d: %s", clients[pos].id, extract);
		broadcast(clients, msg, count, bytes, clients[pos].id);
		free(extract);
		free(msg);
	}
}

int main(int argc, char **argv) {
	int sockfd, connfd, len, capacity, count, port, running, maxfd;
	struct sockaddr_in servaddr, cli;
	t_cli	*clients;

	if (argc != 2)
	{
		write(2, "Wrong number of arguments\n", 26); 
		exit(1);
	}	
	sockfd = 0;
	capacity = 0;
	count = 0;
	port = atoi(argv[1]);
	running = 42;
	maxfd = 0;
	clients = calloc(1, sizeof(t_cli));

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
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		emit_error(sockfd, clients, count);
	if (listen(sockfd, 10) != 0)
		emit_error(sockfd, clients, count);

	while (running)
	{
		fd_set readfds;
		maxfd = sockfd;
		
		FD_ZERO(&readfds);
		FD_SET(sockfd, &readfds);
		for (int i = 0; i < count; i++) {
			FD_SET(clients[i].sockfd, &readfds);
			if (clients[i].sockfd > maxfd)
				maxfd = clients[i].sockfd;
		}
		if (select(maxfd + 1, &readfds, NULL, NULL, 0) < 0)
			emit_error(sockfd, clients, count);
		
		if (FD_ISSET(sockfd, &readfds))
		{
			len = sizeof(cli);
			connfd = accept(sockfd, (struct sockaddr *)&cli, (socklen_t *)&len);
			if (connfd < 0)
				emit_error(sockfd, clients, count);
			t_cli client;
			client.sockfd = connfd;
			client.id = count ? clients[count -1].id + 1 : 0;
			client.buf = NULL;
			capacity = capacity ? capacity * 2 : 4;
			clients = realloc(clients, capacity * sizeof(t_cli));
			clients[count++] = client;
			broadcastServerMsg(clients, count, client.id, 0);
		}
		for (int i = 0; i < count; i++) {
			if (FD_ISSET(clients[i].sockfd, &readfds))
			{
				char	buf[4096];
				ssize_t	bytes;

				bytes = recv(clients[i].sockfd, &buf, sizeof(buf), 0);
				if (bytes <= 0)
				{
					broadcastServerMsg(clients, count, clients[i].id, 1);
					close(clients[i].sockfd);
					free(clients[i].buf);
					for (int pos = 0; pos < count; pos++) {
						if (pos <= i)
							continue;
						clients[pos - 1] = clients[pos];
					}
					count--;
				}
				else
				{
					buf[bytes] = '\0';
					if (strcmp(buf, "EXIT\n") == 0)
					{
						running = 0;
						break;
					}
					clients[i].buf = str_join(clients[i].buf, buf);
					broadcastMsg(sockfd, clients, count, i);
				}
			}
		}
	}
	close(sockfd);
	for (int i = 0; i < count; i++) {
		close(clients[i].sockfd);
		free(clients[i].buf);
	}
	free(clients);
	return (0);
}
