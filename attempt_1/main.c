#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct s_client
{
	int	id;
	int	sock_fd;
}	t_client;

typedef struct s_server
{
	int			sock_fd;
	t_client	*clients;
	struct sockaddr_in	addr;
	int			clients_count;
	int			capacity;
}	t_server;

void	broadcast(t_server server, char *msg, int bytes)
{
	for (int i = 0; i < server.clients_count; i++)
		send(server.clients[i].sock_fd, msg, bytes, 0);
}

void	broadcastServerMsg(t_server server, t_client client, int is_disconnect)
{
	char	msg[512];
	int		bytes;

	if (is_disconnect)
		bytes = sprintf(msg, "server: client %d just left\n", client.id);
	else
		bytes = sprintf(msg, "server: client %d just arrived\n", client.id);

	broadcast(server, msg, bytes);
}

int main(int argc, char **argv) {
	int sockfd, connfd, len, port;
	struct sockaddr_in servaddr, cli;

	t_server	server;
	bzero(&server, sizeof(t_server));

	if (argc < 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}
	port = atoi(argv[1]);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		write(2, "Fatal Error\n", 12);
		exit(1);
	}
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(port);
  
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) {
		write(2, "Fatal Error\n", 12);
		exit(1);
	}
	
	if (listen(sockfd, 10) != 0) {
		write(2, "Fatal Error\n", 12);
		exit(1);
	}

	server.sock_fd = sockfd;
	server.clients = calloc(1, sizeof(t_client));
	server.addr = servaddr;

	while (1)
	{
		fd_set	readfds;
		int		max_fd;

		FD_ZERO(&readfds);
		FD_SET(server.sock_fd, &readfds);
		max_fd = server.sock_fd;

		if (select(max_fd + 1, &readfds, NULL, NULL, 0) < 0)
		{
			write(2, "Fatal Error\n", 12);
			exit(1);
		}
		if (FD_ISSET(server.sock_fd, &readfds))
		{
			//ADD_CLIENT
			len = sizeof(cli);
			connfd = accept(sockfd, (struct sockaddr *)&cli, (socklen_t *)&len);
			if (connfd < 0) { 
				write(2, "Fatal Error\n", 12);
				exit(1);
			}
			t_client	client;

			client.sock_fd = connfd;
			client.id = server.clients_count ? server.clients[server.clients_count - 1].id + 1 : 0;

			server.capacity = server.capacity ? server.capacity * 2 : 4;
			server.clients = realloc(server.clients, server.capacity * sizeof(t_client));
			if (!server.clients)
			{
				write(2, "Fatal Error\n", 12);
				exit(1);
			}
			FD_SET(client.sock_fd, &readfds);
			broadcastServerMsg(server, client, 0);
		}
		for (int i = 0; i < server.clients_count; i++)
		{
			//HANDLE CLIENT DATA
			if (FD_ISSET(server.clients[i].sock_fd, &readfds))
			{
				char	buffer[512];
				int		bytes;

				bytes = recv(server.clients[i].sock_fd, buffer, sizeof(buffer), 0);
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
					char	msg[512];
					int		bytes;
					//BROACAST MSG
					"client %d: "
					buffer[bytes] = '\0';



					

				}
			}
		}

	}



}
