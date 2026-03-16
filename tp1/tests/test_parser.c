#include "../src/parser.h"
#include <stdio.h>
#include <string.h>

/* Esse arquivo é apenas para testar se o parser está
 * funcionando. Não é pra incluir na entrega. */

void test_parser(const char *input) {
	printf("\n>>> Testando: \"%s\"\n", input);
	parsed_command *cmd = parse_command(input);
	print_parsed_command(cmd);
	free_parsed_command(cmd);
}

int main() {
	printf("Testes do parser\n");

	// testes básicos
	test_parser("GET usuario");
	test_parser("SET nome \"João Silva\"");
	test_parser("DELETE id 123");
	test_parser("LIST usuarios, produtos");
	test_parser("SET idade 25, cidade \"São Paulo\"");
	test_parser("HELP");

	test_parser("SET quantidade 100");
	test_parser("GET item 42");

	test_parser("SET mensagem \"Olá, mundo!\"");
	test_parser("SET descricao 'Teste com aspas simples'");

	test_parser("");
	test_parser("comando_invalido arg1 arg2");
	test_parser("SET string_nao_fechada \"teste");

	// modo REPL
	printf("Modo REPL\n");
	printf("Digite comandos (quit para sair)\n");

	char input[1024];
	while (1) {
		printf("> ");
		fflush(stdout);

		if (fgets(input, sizeof(input), stdin) == NULL)
			break;

		// remove newline
		input[strcspn(input, "\n")] = 0;

		if (strlen(input) == 0)
			continue;

		parsed_command *cmd = parse_command(input);

		if (cmd && cmd->is_valid) {
			if (strcmp(cmd->command, "QUIT") == 0 || strcmp(cmd->command, "EXIT") == 0) {
				printf("Saindo...\n");
				free_parsed_command(cmd);
				break;
			}
			print_parsed_command(cmd);
		} else if (cmd)
			printf("ERRO: %s\n", cmd->error_msg ? cmd->error_msg : "Comando inválido");

		free_parsed_command(cmd);
	}

	return 0;
}
