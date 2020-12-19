#include "connection.h"
#include "console_app_tools.h"
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int frontend_fd;
int is_disposed = 0;
struct addrinfo *backend_ai;
Connection *connections;
#define MAX_WAIT_ROOM 1080

// select_loop.c
// Updating select mask and select processing.
void select_loop(Connection **connections,
				 int frontend_fd, void (*on_client_connect)());

void getaddrinfo_or_except(char *url, char *port, struct addrinfo *hints, struct addrinfo **res)
{
	int ret = getaddrinfo(url, port, hints, res);
	if (ret)
	{
		fprintf(stderr, "%sFailed to getaddrinfo: %s\n", RED_COLOR, gai_strerror(ret));
		exit(EXIT_FAILURE);
	}
}

int get_socket_fd_or_except(struct addrinfo *ai,
							int (*bind_or_connect)(int, const struct sockaddr *, socklen_t))
{
	while (1)
	{
		if (!ai){
            throw_and_exit("get_socket_fd");
		}

		int fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (-1 == fd){
            continue;
		}


		if (!bind_or_connect(fd, ai->ai_addr, ai->ai_addrlen)){
            return fd;
		}

		close(fd);
		ai = ai->ai_next;
	}
}

void on_client_connect()
{
	int client_fd = accept(frontend_fd, NULL, NULL);
	if (-1 == client_fd){
        throw_and_exit("accept");
	}

	int backend_fd = get_socket_fd_or_except(backend_ai, connect);
	if (!connection_create(client_fd, backend_fd, &connections)){
        throw_and_exit("connection_create");
	}

}

void at_exit()
{
	if (is_disposed){
        return;
	}

	is_disposed = 1;
	freeaddrinfo(backend_ai);
	close(frontend_fd);
	printf("%sStopped listenning.\n", GREEN_COLOR);
}
/*|->+frontend_port+translatory(our program)->+backend_url(dns->ip)::backend_port+server
  |/
  */
int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		fprintf(stderr, "Usage: %s [frontend_port] [backend_url] [backend_port]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	atexit(at_exit);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, at_exit);
	signal(SIGTERM, at_exit);

	char *frontend_port = argv[1];
	char *backend_url = argv[2];
	char *backend_port = argv[3];

	connections = NULL;

	struct addrinfo hints, *frontend_ai;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	getaddrinfo_or_except(backend_url, backend_port, &hints, &backend_ai);

	hints.ai_flags = AI_PASSIVE;
	getaddrinfo_or_except(NULL, frontend_port, &hints, &frontend_ai);
	frontend_fd = get_socket_fd_or_except(frontend_ai, bind);
	freeaddrinfo(frontend_ai);
	if (listen(frontend_fd, MAX_WAIT_ROOM)){
        throw_and_exit("listen");
	}

	printf("%sStarted listening\n", YELLOW_COLOR);

	select_loop(&connections, frontend_fd, on_client_connect);

	exit(EXIT_SUCCESS);
}
