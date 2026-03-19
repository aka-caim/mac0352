#include "tcp_server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
	int server_socket, client_socket;
	struct sockaddr_in server_addr, client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	int opt = 1;

	// create socket
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0)
		error_exit("Erro ao criar socket");

	// configure socket to reuse address
	if (setsockopt(server_socket, SOL_SOCKET,
				   SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		error_exit("Erro ao configurar socket");

	// configure server address
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	int bind_status;
	if ( (bind_status = bind(server_socket,
							 (struct sockaddr *) &server_addr,
							 sizeof(server_addr))) < 0)
		error_exit("Erro no bind");

	int listen_status;
	if ( (listen_status = listen(server_socket, MAX_CLIENTS)) < 0)
		error_exit("Erro no listen");

	printf("Servidor TCP iniciado na porta %d\n", PORT);
	printf("Aguardando conexões...\n\n");

	// main loop
	while (1) {
		// accept connection
		client_socket = accept(server_socket,
							   (struct sockaddr *) &client_addr,
							   &client_addr_len);

		if (client_socket < 0) {
			perror("Erro ao aceitar conexão");
			continue; // process next connection1
		}

		printf("Nova conexão de %s:%d\n", 
				inet_ntoa(client_addr.sin_addr), 
				ntohs(client_addr.sin_port));

		/* TODO = implementar versão não-bloqueante desse loop.
		 * Temos que decidir se usar threads, processos ou I/O
		 * multiplexado*/
		handle_client(client_socket);

		printf("Conexão encerrada com %s:%d\n\n", 
				inet_ntoa(client_addr.sin_addr), 
				ntohs(client_addr.sin_port));
	}

	close(server_socket);
	return 0;
}

void handle_client(int client_socket) {
	char buffer[BUFFER_SIZE];
	ssize_t bytes_received;

	// sending welcome message
	const char *welcome = "Conectado ao servidor TCP\n";
	send(client_socket, welcome, strlen(welcome), 0);

	// main loop
	while (1) {
		memset(buffer, 0, BUFFER_SIZE);
		bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

		if (bytes_received <= 0) {
			if (bytes_received == 0) printf("Cliente desconectado\n");
			else perror("Erro ao receber dados");
			break;
		}

		// remove line break
		buffer[strcspn(buffer, "\r\n")] = 0;

		printf("Recebido: %s\n", buffer);

		// output command
		if (strcmp(buffer, "quit") == 0 || strcmp(buffer, "exit") == 0) {
			const char *goodbye = "Encerrando conexão...\n";
			send(client_socket, goodbye, strlen(goodbye), 0);
			break;
		}

		// echoing message from client
		char response[BUFFER_SIZE + 50];
		snprintf(response, sizeof(response), "Echo: %s\n", buffer);
		send(client_socket, response, strlen(response), 0);
	}

	close(client_socket);
}

void error_exit(const char *message) {
	perror(message);
	exit(EXIT_FAILURE);
}
