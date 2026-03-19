#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#define PORT 6767
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 5

void handle_client(int client_socket);
void error_exit(const char *message);

#endif // TCP_SERVER_H
