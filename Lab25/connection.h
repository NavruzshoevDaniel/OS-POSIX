#ifndef CONNECTION
#define CONNECTION

#include <time.h>

#define BUFSIZE 16000

typedef struct Connection
{
	unsigned id;
	struct Connection *prev, *next;

	int client_fd;
	int backend_fd;

	char client_to_backend_bytes[BUFSIZE];
	char backend_to_client_bytes[BUFSIZE];
	size_t client_to_backend_bytes_count;
	size_t backend_to_client_bytes_count;

	int is_broken;

	time_t last_update;
} Connection;

Connection *connection_create(int client_fd, int backend_fd, Connection **connections);
void connection_drop(Connection *connection, Connection **connections);

#endif