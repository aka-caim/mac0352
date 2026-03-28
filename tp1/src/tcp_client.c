#include "tcp_client.h"

/* Parse a TCP port from CLI input.
 * Returns 0 on success and stores the parsed value into port_out.
 */
static int parse_port(const char *raw_port, int *port_out)
{
	char *endptr = NULL;
	long value = strtol(raw_port, &endptr, 10);

	if (!raw_port || raw_port[0] == '\0' || *endptr != '\0')
		return -1;

	if (value < 1 || value > 65535)
		return -1;

	*port_out = (int)value;
	return 0;
}

/* Send the full buffer to the socket, handling partial send() calls.
 */
static int send_all(int sock_fd, const char *buffer, size_t len)
{
	size_t sent_total = 0;

	while (sent_total < len) {
		ssize_t sent_now = send(sock_fd, buffer + sent_total, len - sent_total, 0);
		if (sent_now < 0)
			return -1;
		sent_total += (size_t)sent_now;
	}

	return 0;
}

/* Write the full buffer to stdout, handling partial write() calls.
 */
static int write_all_stdout(const char *buffer, size_t len)
{
	size_t written_total = 0;

	while (written_total < len) {
		ssize_t written_now = write(STDOUT_FILENO, buffer + written_total,
					len - written_total);
		if (written_now < 0)
			return -1;
		written_total += (size_t)written_now;
	}

	return 0;
}

void error_exit(const char *message) {
	if (errno != 0) {
		syslog(LOG_CRIT, "client: %s: %s", message, strerror(errno));
		perror(message);
	} else {
		syslog(LOG_CRIT, "client: %s", message);
		fprintf(stderr, "Error: %s\n", message);
	}
	closelog();
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
	int client_socket;
	struct sockaddr_in server_addr;
	char buffer[BUFFER_SIZE];
	int stdin_open = 1;

	const char *server_ip = "127.0.0.1";
	int port = DEFAULT_PORT;

	// initialize process logging for the client
	openlog("TCP Client", LOG_PID | LOG_CONS, LOG_USER);

	// optional CLI overrides: <server_ip> <port>.
	if (argc > 1) server_ip = argv[1];
	if (argc > 2 && parse_port(argv[2], &port) != 0) {
		syslog(LOG_ERR, "client: invalid port value '%s'", argv[2]);
		fprintf(stderr, "Error: invalid port value '%s' (expected 1-65535).\n",
						argv[2]);
		closelog();
		return EXIT_FAILURE;
	}

	/* 1. socket */
	client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client_socket < 0)
		error_exit("Failed to create socket");

	// configure target server address
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	// configure optional ip received via CLI
	if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
		syslog(LOG_ERR, "client: invalid IPv4 address '%s'", server_ip);
		fprintf(stderr, "Error: invalid IPv4 address '%s'.\n", server_ip);
		close(client_socket);
		closelog();
		return EXIT_FAILURE;
	}

	/* 2. connect */

	// user-facing connection status is printed to stdout
	printf("Connecting to server %s:%d...\n", server_ip, port);
	syslog(LOG_INFO, "client: connecting to server %s:%d", server_ip, port);
	if (connect(client_socket, (struct sockaddr *)&server_addr,
					sizeof(server_addr)) < 0)
		error_exit("Failed to connect to server");

	printf("Connected successfully.\n");
	printf("Type text and press Enter (Ctrl+D closes input).\n\n");
	syslog(LOG_INFO, "client: connected to server %s:%d", server_ip, port);

	/* 3. main loop */
	while (1) {
		fd_set read_fds;
		int max_fd = client_socket;

		FD_ZERO(&read_fds);
		if (stdin_open) {
			FD_SET(STDIN_FILENO, &read_fds);
			if (STDIN_FILENO > max_fd)
				max_fd = STDIN_FILENO;
		}
		FD_SET(client_socket, &read_fds);

		if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
			if (errno == EINTR)
				continue;
			syslog(LOG_ERR, "client: select() failed: %s", strerror(errno));
			perror("Error: select() failed");
			break;
		}

		// stdin became readable: consume and send bytes to server
		if (stdin_open && FD_ISSET(STDIN_FILENO, &read_fds)) {
			ssize_t bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer));
			if (bytes_read < 0) {
				syslog(LOG_ERR, "client: failed to read stdin: %s", strerror(errno));
				perror("Error: failed to read stdin");
				break;
			}

			// EOF on stdin: close only the write half of the TCP connection
			if (bytes_read == 0) {
				if (shutdown(client_socket, SHUT_WR) < 0) {
					syslog(LOG_ERR, "client: shutdown(SHUT_WR) failed: %s",
								strerror(errno));
					perror("Error: failed to shutdown write side");
				}
				syslog(LOG_INFO, "%s",
							"client: stdin closed; write side shutdown requested");
				stdin_open = 0;
			}

			if (bytes_read > 0)
				if (send_all(client_socket, buffer, (size_t)bytes_read) < 0) {
					syslog(LOG_ERR, "client: failed to send data: %s", strerror(errno));
					perror("Error: failed to send message");
					close(client_socket);
					closelog();
					return EXIT_FAILURE;
				}
		}

		// socket became readable: receive and flush bytes to stdout
		if (FD_ISSET(client_socket, &read_fds)) {
			ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
			if (bytes_received < 0) {
				syslog(LOG_ERR, "client: failed to receive data: %s", strerror(errno));
				perror("Error: failed to receive response");
				break;
			}

			if (bytes_received == 0) {
				printf("Server disconnected\n");
				syslog(LOG_INFO, "%s", "client: server closed the connection");
				break;
			}

			if (write_all_stdout(buffer, (size_t)bytes_received) < 0) {
				syslog(LOG_ERR, "client: failed to write to stdout: %s", strerror(errno));
				perror("Error: failed to write output");
				close(client_socket);
				closelog();
				return EXIT_FAILURE;
			}
		}
	}

	/* 4. close */
	close(client_socket);
	printf("Connection closed.\n");
	syslog(LOG_INFO, "%s", "client: terminated");
	closelog();

	return 0;
}
