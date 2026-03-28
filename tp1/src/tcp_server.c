#include "tcp_server.h"

#define MAX_LINE 1024
#define INITIAL_POLL_SIZE 10
#define MAX_CLIENTS 1000
#define	SA struct sockaddr

void error_exit(const char *message) {
	syslog(LOG_CRIT, "server: %s: %s", message, strerror(errno));
	exit(EXIT_FAILURE);
}

static void handle_new_connection(int listen_fd, struct pollfd **poll_fds,
												 int *nfds, int *poll_size)
{
	struct sockaddr_in client_addr;
	socklen_t cli_sock_len = sizeof(client_addr);

	int conn_fd = accept(listen_fd, (SA *) &client_addr, &cli_sock_len);
	if (conn_fd < 0) {
		syslog(LOG_ERR, "Error accepting connection: %s", strerror(errno));
		return;
	}

	// expand the pollfd array when needed
	if (*nfds >= *poll_size) {
		*poll_size *= 2;
		*poll_fds = realloc(*poll_fds, *poll_size * sizeof(struct pollfd));
		if (!*poll_fds)
			error_exit("Error: realloc failed");
	}

	// add the new client to the poll-monitored set
	(*poll_fds)[*nfds].fd = conn_fd;
	(*poll_fds)[*nfds].events = POLLIN;
	(*nfds)++;

	char client_ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
	syslog(LOG_INFO, "server: client connected %s:%d (fd=%d)", 
			client_ip, ntohs(client_addr.sin_port), conn_fd);

	// send a cute greeting for interactive clients
	const char *welcome = "Server is up and running :3\n";
	if (write(conn_fd, welcome, strlen(welcome)) < 0)
		syslog(LOG_ERR, "server: failed to write welcome to fd=%d: %s",
					conn_fd, strerror(errno));
}

static void handle_client_data(int index, struct pollfd **poll_fds, int *nfds)
{
	int sock_fd = (*poll_fds)[index].fd; // client fd
	char buffer[MAX_LINE], response[MAX_LINE];
	response[0] = '\0';

	// check descriptor errors and disconnection events
	if ((*poll_fds)[index].revents & (POLLERR | POLLHUP)) {
		syslog(LOG_INFO, "server: client fd=%d disconnected (poll error/hangup)",
					sock_fd);
		goto close_connection;
	}

	// read data sent by the client
	ssize_t n = read(sock_fd, buffer, MAX_LINE - 1);

	if (n < 0) {
		syslog(LOG_ERR, "server: failed to read from fd=%d: %s",
					sock_fd, strerror(errno));
		goto close_connection;
	} else if (n == 0) {
		// connection closed by the client
		syslog(LOG_INFO, "server: client fd=%d closed connection", sock_fd);
		goto close_connection;
	}

	buffer[n] = '\0';

	// strip trailing newline characters from the received line
	char *newline = strchr(buffer, '\n');
	if (newline) *newline = '\0';
	newline = strchr(buffer, '\r');
	if (newline) *newline = '\0';

	syslog(LOG_DEBUG, "server: received from fd=%d (%zd bytes): '%s'",
				sock_fd, n, buffer);

	// parse and validate command and arguments
	parsed_command *cmd = parse_command(buffer);

	if (!cmd) {
		syslog(LOG_ERR, "server: could not allocate parsed command for fd=%d",
					sock_fd);
		snprintf(response, sizeof(response),
					"ERROR: Internal failure while processing command.\n");
	} else if (!cmd->is_valid) {
		syslog(LOG_WARNING, "server: invalid command from fd=%d: %s", sock_fd,
				cmd->error_msg ? cmd->error_msg : "unknown parse error");
		snprintf(response, sizeof(response), "ERROR: %s\n",
				cmd->error_msg ? cmd->error_msg : "Invalid command");
	} else {
		int result = execute_command(cmd, response, MAX_LINE);
		if (result != 0) {
			syslog(LOG_WARNING, "server: command failed for fd=%d: %s",
						sock_fd, cmd->command);
			if (response[0] == '\0')
				snprintf(response, sizeof(response),
						"ERROR: Command execution failed.\n");
		}
	}

	free_parsed_command(cmd);

	// send response back to the client
	if (write(sock_fd, response, strlen(response)) < 0) {
		syslog(LOG_ERR, "server: failed to write to fd=%d: %s",
					sock_fd, strerror(errno));
		goto close_connection;
	}

	return;

/* yeah, this is a go-to. nothing to worry about.
 * https://c-faq.com/style/stylewars.html
 * it is all over the linux kernel too */

// single close block to avoid duplicated cleanup logic
close_connection:
	syslog(LOG_INFO, "server: closing connection fd=%d", sock_fd);
	close(sock_fd);
	(*poll_fds)[index].fd = -1;

	// shrink array by trimming inactive descriptors at the end
	while (*nfds > 1 && (*poll_fds)[*nfds - 1].fd < 0)
		(*nfds)--;
}

int execute_command(const parsed_command *cmd, char *response,
											size_t response_size)
{
    if (!cmd || !cmd->command) {
		syslog(LOG_ERR, "server: null command object");
        return -1;
    }
    
	syslog(LOG_DEBUG, "server: executing command %s with %d argument(s)",
					  cmd->command, cmd->arg_count);
    
    /* CREATE <key> */
    if (strcmp(cmd->command, "CREATE") == 0) {
        if (cmd->arg_count < 1) {
			syslog(LOG_ERR, "server: CREATE requires a key");
            return -1;
        }

		// TODO: implement CREATE here

		snprintf(response, response_size,
						"OK: CREATE %s (not implemented)\n", cmd->args[0]);
        return 0;
    }
    
	/* GET <key> */
    else if (strcmp(cmd->command, "GET") == 0) {
        if (cmd->arg_count < 1) {
			syslog(LOG_ERR, "server: GET requires a key");
            return -1;
        }

        // TODO: implement GET here

		snprintf(response, response_size,
						"OK: GET %s (not implemented)\n", cmd->args[0]);
        return 0;
    }

    /* SET <key> <value> */
    else if (strcmp(cmd->command, "SET") == 0) {
        if (cmd->arg_count < 2) {
			syslog(LOG_ERR, "server: SET requires a key and a value");
            return -1;
        }

        // TODO: implement SET here

		snprintf(response, response_size,
					"OK: SET %s = %s (not implemented)\n",
					cmd->args[0], cmd->args[1]);
        return 0;
    }
    
	/* RESERVE <resource> */
	else if (strcmp(cmd->command, "RESERVE") == 0) {
        if (cmd->arg_count < 1) {
			syslog(LOG_ERR, "server: RESERVE requires a resource");
            return -1;
        }

		// TODO: implement RESERVE here

		snprintf(response, response_size,
					"OK: RESERVE %s (not implemented)\n", cmd->args[0]);
		return 0;
	}

	/* RELEASE <resource> */
	else if (strcmp(cmd->command, "RELEASE") == 0) {
		if (cmd->arg_count < 1) {
			syslog(LOG_ERR, "server: RELEASE requires a resource");
			return -1;
		}

		// TODO: implement RELEASE here

		snprintf(response, response_size,
					"OK: RELEASE %s (not implemented)\n", cmd->args[0]);
        return 0;
    }
    
    /* LIST */
    else if (strcmp(cmd->command, "LIST") == 0) {
		// TODO: implement LIST logic here
		snprintf(response, response_size, "OK: LIST (not implemented)\n");
        return 0;
    }
    
	// unknown command
    else {
		snprintf(response, response_size,
					"ERROR: Command '%s' is not recognized.\n", cmd->command);
        return -1;
    }
}

int main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	// initialize process logging for the server
	openlog("TCP Server", LOG_PID | LOG_CONS, LOG_USER);

	int listen_fd;
	struct sockaddr_in server_addr;

	/* 1. socket */
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0)
		error_exit("Error: could not create socket");

	// allow address reuse for fast restarts
	int opt = 1;
	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		error_exit("Error: setsockopt failed");

	// configure local server address
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY; 
	server_addr.sin_port = htons(PORT);

	/* 2. bind */
	if (bind(listen_fd, (struct sockaddr *)&server_addr,
						sizeof(server_addr)) < 0)
		error_exit("Error: could not bind address to socket");
	syslog(LOG_DEBUG, "%s",
			"Server address successfully bound to listening socket");

	/* 3. listen */
	if (listen(listen_fd, MAX_CLIENTS) < 0)
		error_exit("Error: could not set server to listening mode");
	syslog(LOG_DEBUG, "%s", "Server set to listen mode");

	// initialize poll structure used for connection multiplexing
	int poll_size = INITIAL_POLL_SIZE;
	struct pollfd *poll_fds = malloc(poll_size * sizeof(struct pollfd));
	if (!poll_fds)
		error_exit("Error: malloc of poll_fds failed");

	poll_fds[0].fd = listen_fd;
	poll_fds[0].events = POLLIN;
	int nfds = 1;

	syslog(LOG_DEBUG, "%s", "server: poll data structure initialized");

	syslog(LOG_INFO, "server: TCP server started on port %d", PORT);

	/* 4. main loop for accepting and processing connections */
	while (1) {
		/* 4.1. Poll */
		int n_ready = poll(poll_fds, nfds, -1); // infinite timeout
		if (n_ready < 0) {
			if (errno == EINTR) continue; // interrupted by signal
			error_exit("Error: poll failed");
		}

		// check for new incoming connections on the listening socket
		if (poll_fds[0].revents & POLLIN) {
			syslog(LOG_DEBUG, "%s", "server: new incoming connection detected");
			handle_new_connection(listen_fd, &poll_fds, &nfds, &poll_size);
			n_ready--;
		}

		// process clients with pending I/O events
		for (int i = 1; i < nfds && n_ready > 0; i++) {
			if (poll_fds[i].fd < 0) continue;

			/* POLLIN	Alert me when data is ready to recv() on this socket
			   POLLERR	An error has occurred on this socket
			   POLLHUP	The remote side of the connection hung up */
			if (poll_fds[i].revents & (POLLIN | POLLERR | POLLHUP)) {
				/* 4.2. process data/state for active clients. */
				handle_client_data(i, &poll_fds, &nfds);
				n_ready--;
			}
		}
	}

	/* 5. close (and cleanup) */
	for (int i = 0; i < nfds; i++)
		if (poll_fds[i].fd >= 0)
			close(poll_fds[i].fd);

	free(poll_fds);
	closelog();

	return EXIT_SUCCESS;
}
