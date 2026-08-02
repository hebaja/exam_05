#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>

void	emit_fatal_error()
{
	write(2, "Fatal error\n", 12);
	exit(1);
}

typedef struct s_serv
{
	int					sock_fd;
	struct pollfd		fds[2];
	struct sockaddr_in	addr;
} t_serv;

void	set_server(t_serv *server)
{
	int	opt = 1;

	server->sock_fd = socket(AF_INET, SOCK_STREAM, 0);	
	server->addr.sin_family = AF_INET;
	server->addr.sin_port = htons(8080);
	server->addr.sin_addr.s_addr = INADDR_ANY;
	memset(server->addr.sin_zero, 0, sizeof(server->addr.sin_zero));
	setsockopt(server->sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

int	main()
{
	t_serv	server;

	set_server(&server);	

	if (bind(server.sock_fd, (struct sockaddr*)&server.addr, sizeof(server.addr)) < 0)
		emit_fatal_error();
	if (listen(server.sock_fd, 10) < 0)
		emit_fatal_error();

	server.fds[0].fd = server.sock_fd;
	server.fds[0].events = POLLIN;
	server.fds[1].fd = -1;
	server.fds[1].events = POLLIN;



	while (1)
	{
		poll(server.fds, 2, -1);
		if (server.fds[0].revents & POLLIN)
		{
			struct sockaddr_in	client_addr;
			socklen_t			addr_len;

			addr_len = sizeof(client_addr);

			int	client_fd = accept(server.sock_fd, (struct sockaddr*)&client_addr, &addr_len);

			server.fds[1].fd = client_fd;
			server.fds[1].events = POLLIN;

			printf("client connected %d\n", client_fd);

		}
		if (server.fds[1].revents & POLLIN)
		{
			char	buffer[512];
			
			int bytes = recv(server.fds[1].fd, buffer, sizeof(buffer), 0);

			buffer[bytes] = '\0';

			printf("%s", buffer);

		}
	}


	return (0);
}
