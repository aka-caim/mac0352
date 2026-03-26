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

#define BUFFER_SIZE 1024
#define DEFAULT_PORT 6767

void error_exit(const char *message);

int main(int argc, char *argv[]) {
	int client_socket;
	struct sockaddr_in server_addr;
	char buffer[BUFFER_SIZE];
	int stdin_open = 1;

	const char *server_ip = "127.0.0.1";
	int port = DEFAULT_PORT;

	if (argc > 1)
		server_ip = argv[1];
	if (argc > 2)
		port = atoi(argv[2]);

	// create socket
	client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client_socket < 0)
		error_exit("Erro ao criar socket");

	// configure server address
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
		error_exit("Endereço IP inválido");

	// connect to the server
	printf("Conectando ao servidor %s:%d...\n", server_ip, port);
	if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
		error_exit("Erro ao conectar ao servidor");

	printf("Conectado com sucesso!\n");
	printf("Digite texto e pressione Enter (Ctrl+D para encerrar entrada).\n\n");

	// Multiplexa entrada do teclado e respostas do servidor.
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
			perror("Erro no select");
			break;
		}

		if (stdin_open && FD_ISSET(STDIN_FILENO, &read_fds)) {
			ssize_t bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer));
			if (bytes_read < 0) {
				perror("Erro ao ler entrada");
				break;
			}

			if (bytes_read == 0) {
				if (shutdown(client_socket, SHUT_WR) < 0)
					perror("Erro ao encerrar envio");
				stdin_open = 0;
			}

			if (bytes_read > 0) {
				ssize_t sent_total = 0;
				while (sent_total < bytes_read) {
					ssize_t sent_now = send(client_socket, buffer + sent_total, bytes_read - sent_total, 0);
					if (sent_now < 0) {
						perror("Erro ao enviar mensagem");
						close(client_socket);
						return EXIT_FAILURE;
					}
					sent_total += sent_now;
				}
			}
		}

		if (FD_ISSET(client_socket, &read_fds)) {
			ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
			if (bytes_received < 0) {
				perror("Erro ao receber resposta");
				break;
			}

			if (bytes_received == 0) {
				printf("Servidor desconectado\n");
				break;
			}

			ssize_t written_total = 0;
			while (written_total < bytes_received) {
				ssize_t written_now = write(STDOUT_FILENO, buffer + written_total, bytes_received - written_total);
				if (written_now < 0) {
					perror("Erro ao escrever saída");
					close(client_socket);
					return EXIT_FAILURE;
				}
				written_total += written_now;
			}
		}
	}

	close(client_socket);
	printf("Conexão encerrada.\n");

	return 0;
}

void error_exit(const char *message) {
	perror(message);
	exit(EXIT_FAILURE);
}
