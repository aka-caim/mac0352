#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>

#define BUFFER_SIZE 1024
#define DEFAULT_PORT 6767

void error_exit(const char *message);

#endif // TCP_CLIENT_H