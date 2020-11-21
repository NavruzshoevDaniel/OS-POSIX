#include "connection.h"
#include "console_app_tools.h"
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define NO_UPDATE_LIMIT_SEC 20
#define SELECT_TIMEOUT_SEC 100

void time_or_expect(time_t *tloc) {
    if (-1 == time(tloc)) {
        throw_and_exit("time");
    }

}

void update_max_fd_or_except(int fd, int *max_fd) {
    if (fd + 1 >= FD_SETSIZE) {
        fprintf(stderr, "%sFailed to update_max_fd: fd > FD_SETSIZE\n", RED_COLOR);
        exit(EXIT_FAILURE);
    }
    if (fd > *max_fd) {
        *max_fd = fd;
    }
}

void select_loop(Connection **connections,
                 int frontend_fd, void (*on_client_connect)()) {
    fd_set readfds, writefds;
    int ready, read;
    char buf[BUFSIZE];
    Connection *connection;
    time_t time_now;
    struct timeval timeout;
    timeout.tv_sec = SELECT_TIMEOUT_SEC;
    timeout.tv_usec = 0;
    int a = 0;
    int count=0;
    while (a != 1) {
        int max_fd = 0;
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_SET(frontend_fd, &readfds);
        update_max_fd_or_except(frontend_fd, &max_fd);

        for (Connection *connection = *connections; connection; connection = connection->next) {
            time_or_expect(&time_now);

            if (connection->is_broken) {
                connection_drop(connection, connections);
            } else if (time_now - connection->last_update > NO_UPDATE_LIMIT_SEC) {
                connection_drop(connection, connections);
            } else {
                update_max_fd_or_except(connection->client_fd, &max_fd);
                update_max_fd_or_except(connection->backend_fd, &max_fd);
                if (connection->client_to_backend_bytes_count) {
                    FD_SET(connection->backend_fd, &writefds);
                } else {
                    FD_SET(connection->client_fd, &readfds);
                }


                if (connection->backend_to_client_bytes_count) {
                    FD_SET(connection->client_fd, &writefds);
                } else {
                    FD_SET(connection->backend_fd, &readfds);
                }

            }
        }

        ready = select(max_fd + 1, &readfds, &writefds, NULL, &timeout);
        if (-1 == ready) {
            throw_and_exit("select");
        } else if (!ready) {
            continue;
        }


        for (Connection *connection = *connections; connection; connection = connection->next) {
            int connection_is_updated = 0;
            printf("%sSalamalekum:%d\n", GREEN_COLOR,count++);
            // Client -> Frontend
            if (FD_ISSET(connection->client_fd, &readfds)) {
                int bytes_received =
                        recv(connection->client_fd, connection->client_to_backend_bytes,
                             sizeof(connection->client_to_backend_bytes), 0);
                connection->client_to_backend_bytes_count = bytes_received;
                if (bytes_received) {
                    connection_is_updated = 1;
                }

            }

            // Frontend -> Backend
            if (FD_ISSET(connection->backend_fd, &writefds)) {
                if (-1 == send(connection->backend_fd, connection->client_to_backend_bytes,
                               connection->client_to_backend_bytes_count, 0)) {
                    connection->is_broken = 1;
                    continue;
                }
                connection->client_to_backend_bytes_count = 0;
                connection_is_updated = 1;
            }

            // Backend -> Frontend
            if (FD_ISSET(connection->backend_fd, &readfds)) {
                int bytes_received =
                        recv(connection->backend_fd, connection->backend_to_client_bytes,
                             sizeof(connection->backend_to_client_bytes), 0);
                connection->backend_to_client_bytes_count = bytes_received;
                if (bytes_received) {
                    connection_is_updated = 1;
                }
            }

            // Backend -> Client
            if (FD_ISSET(connection->client_fd, &writefds)) {
                if (-1 == send(connection->client_fd, connection->backend_to_client_bytes,
                               connection->backend_to_client_bytes_count, 0)) {
                    connection->is_broken = 1;
                    continue;
                }
                connection->backend_to_client_bytes_count = 0;
                connection_is_updated = 1;
            }
            if (connection_is_updated) {
                time_or_expect(&connection->last_update);
            }

        }

        // Client connect
        if (FD_ISSET(frontend_fd, &readfds)) {
            on_client_connect();
        }

    }
}