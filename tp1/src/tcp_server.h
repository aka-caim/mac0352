#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "parser.h"
#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <errno.h>

#define PORT 6767
#define MAX_LINE 1024
#define INITIAL_POLL_SIZE 10

int execute_command(const parsed_command *cmd, char *response, size_t response_size);

#endif // TCP_SERVER_H
