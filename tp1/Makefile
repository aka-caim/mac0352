CC = gcc
CFLAGS = -Wall -Wextra -std=c17 -g
LDFLAGS = 

# main targets
all: tcp_server tcp_client test_parser

# Servidor TCP
tcp_server: tcp_server.o
	$(CC) $(CFLAGS) -o tcp_server tcp_server.o $(LDFLAGS)

tcp_server.o: tcp_server.c
	$(CC) $(CFLAGS) -c tcp_server.c

# Cliente TCP
tcp_client: tcp_client.o
	$(CC) $(CFLAGS) -o tcp_client tcp_client.o $(LDFLAGS)

tcp_client.o: tcp_client.c
	$(CC) $(CFLAGS) -c tcp_client.c

# Parser de comandos
test_parser: test_parser.o command_parser.o
	$(CC) $(CFLAGS) -o test_parser test_parser.o parser.o $(LDFLAGS)

test_parser.o: test_parser.c command_parser.h
	$(CC) $(CFLAGS) -c test_parser.c

command_parser.o: command_parser.c command_parser.h
	$(CC) $(CFLAGS) -c parser.c

clean:
	rm -f *.o tcp_server tcp_client test_parser

# help targets
run_server: tcp_server
	./tcp_server

run_client: tcp_client
	./tcp_client

run_parser: test_parser
	./test_parser

# Ajuda
help:
	@echo "Available targets:"
	@echo "  make all         - Compila todos os programas"
	@echo "  make tcp_server  - Compila apenas o servidor TCP"
	@echo "  make tcp_client  - Compila apenas o cliente TCP"
	@echo "  make test_parser - Compila o teste do parser"
	@echo "  make clean       - Remove arquivos compilados"
	@echo "  make run_server  - Compila e executa o servidor"
	@echo "  make run_client  - Compila e executa o cliente"
	@echo "  make run_parser  - Compila e executa o teste do parser"

.PHONY: all clean run_server run_client run_parser help
