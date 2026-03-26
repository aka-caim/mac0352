#include "tcp_server.h"

#define MAX_LINE 1024
#define INITIAL_POLL_SIZE 10
#define MAX_CLIENTS 1000
#define	SA struct sockaddr

void error_exit(const char *message) {
	perror(message);
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

	// expand array if needed
	if (*nfds >= *poll_size) {
		*poll_size *= 2;
		*poll_fds = realloc(*poll_fds, *poll_size * sizeof(struct pollfd));
		if (!*poll_fds)
			error_exit("Error: realloc failed");
	}

	// add new client
	(*poll_fds)[*nfds].fd = conn_fd;
	(*poll_fds)[*nfds].events = POLLIN;
	(*nfds)++;

	char client_ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
	printf("Nova conexão de %s:%d (fd=%d, total=%d clientes)\n", 
			client_ip, ntohs(client_addr.sin_port), conn_fd, *nfds - 1);

	syslog(LOG_INFO, "Client connected: %s:%d (fd=%d)", 
			client_ip, ntohs(client_addr.sin_port), conn_fd);

	// send welcome message
	const char *welcome = "Servidor está no ar\n";
	write(conn_fd, welcome, strlen(welcome));
}

static void handle_client_data(int index, struct pollfd **poll_fds, int *nfds)
{
	int sock_fd = (*poll_fds)[index].fd; // client fd
	char buffer[MAX_LINE], response[MAX_LINE];

	// check for error and disconnection
	if ((*poll_fds)[index].revents & (POLLERR | POLLHUP)) {
		printf("Cliente fd=%d desconectado (erro ou hangup)\n", sock_fd);
		goto close_connection;
	}

	// read message sent by client
	ssize_t n = read(sock_fd, buffer, MAX_LINE - 1);

	if (n < 0) {
		syslog(LOG_ERR, "Error reading from fd=%d: %s", sock_fd, strerror(errno));
		goto close_connection;
	} else if (n == 0) {
		// connection closed by client
		printf("Cliente fd=%d fechou a conexão\n", sock_fd);
		goto close_connection;
	}

	buffer[n] = '\0';

	char *newline = strchr(buffer, '\n'); // strip newline from the end
	if (newline) *newline = '\0';
	newline = strchr(buffer, '\r');
	if (newline) *newline = '\0';

	printf("Recebido de fd=%d (%zd bytes): '%s'\n", sock_fd, n, buffer);

	// using our custom parser to process client message
	parsed_command *msg = parse_command(buffer);

	if (!msg)
		syslog(LOG_ERR, "Error %d: could not parse command", errno);
	else if (!msg->is_valid)
		printf("Invalid command");
	else {
		int result = execute_command(msg, response, MAX_LINE);

		// if command is quit or exit, mark to disconnection
		if (strcmp(msg->command, "QUIT") == 0 || strcmp(msg->command, "EXIT") == 0) {
			free_parsed_command(msg);
			write(sock_fd, response, strlen(response));
			goto close_connection;
		}
	}

	free_parsed_command(msg);

	// send message
	if (write(sock_fd, response, strlen(response)) < 0) {
		syslog(LOG_ERR, "Error writing to fd=%d: %s", sock_fd, strerror(errno));
		goto close_connection;
	}

	return;

/* sim isso é um goto, mas esse claramente simplifica e o código é claro. não
 * há razão pra ser religiosamente contra uma prática se ela se mostra boa pro
 * objetivo desejado */
close_connection:
	syslog(LOG_INFO, "Closing connection fd=%d", sock_fd);
	close(sock_fd);
	(*poll_fds)[index].fd = -1;

	// contract array removing inactive fds in the end
	while (*nfds > 1 && (*poll_fds)[*nfds - 1].fd < 0)
		(*nfds)--;
}

int execute_command(const parsed_command *cmd, char *response, size_t response_size) {
	/* TODO */

	return 0;
}

int main(int argc, char *argv[])
{
	// for debug purposes
	openlog("Servidor TCP", LOG_PID | LOG_CONS, LOG_USER);

	int listen_fd;
	SA server_addr;

	/* 1. Socket */
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0)
		error_exit("Error: could not create socket");

	// set socket option to reuse address when fail occurs
	int opt = 1;
	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		error_exit("Error: setsockopt failed");

	syslog(LOG_DEBUG, "%s", "Socket listen_fd criado");

	// configure server address
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY; 
	server_addr.sin_port = htons(PORT);

	/* 2. Bind */
	if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
		error_exit("Error: could not bind address to socket");
	syslog(LOG_DEBUG, "%s", "Feito o bind do servidor ao listen_fd");

	/* 3. Listen */
	if (listen(listen_fd, MAX_CLIENTS) < 0)
		error_exit("Error: could not set server to listening mode");
	syslog(LOG_DEBUG, "%s", "Servidor foi colocado em modo listen");

	// initialize pollfd data structure and allocate memory
	int poll_size = INITIAL_POLL_SIZE;
	struct pollfd *poll_fds = malloc(poll_size * sizeof(struct pollfd));
	if (!poll_fds)
		error_exit("Error: malloc of poll_fds failed");

	poll_fds[0].fd = listen_fd;
	poll_fds[0].events = POLLIN;
	int nfds = 1;

	syslog(LOG_DEBUG, "%s", "Estruturas de dados do poll foram inicializadas");

	printf("Servidor TCP iniciado na porta %d\n", PORT);

	/* 4. Main loop for accepting and processing connections */
	int loop_step = 0;
	while (1) {
		loop_step++;

		/* 4.1. Poll */
		int n_ready = poll(poll_fds, nfds, -1); // infinite timeout
		if (n_ready < 0) {
			if (errno == EINTR) continue; /* interrupted by signal */
			error_exit("Error: poll failed");
		}

		printf("[Passo %d] Descritores prontos: %d\n", loop_step, n_ready);

		// check listening socket to see if there's new connections
		if (poll_fds[0].revents & POLLIN) {
			printf("Nova conexão iniciada\n");
			handle_new_connection(listen_fd, &poll_fds, &nfds, &poll_size);
			n_ready--;
		}

		// check the n_ready connections ready to make I/O operation
		for (int i = 1; i < nfds && n_ready > 0; i++) {
			if (poll_fds[i].fd < 0) continue;

			/* POLLIN	Alert me when data is ready to recv() on this socket
			   POLLERR	An error has occurred on this socket
			   POLLHUP	The remote side of the connection hung up */
			if (poll_fds[i].revents & (POLLIN | POLLERR | POLLHUP)) {
				/* 4.2. Accept (handle new connections) */

				printf("Cliente %d enviou dados\n", i);
				handle_client_data(i, &poll_fds, &nfds);
				n_ready--;
			}
		}
	}

	/* 5. Close (and cleanup) */
	for (int i = 0; i < nfds; i++)
		if (poll_fds[i].fd >= 0)
			close(poll_fds[i].fd);
	free(poll_fds);
	closelog();

	return EXIT_SUCCESS;
}
