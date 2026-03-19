#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

typedef enum {
	TOKEN_COMMAND,      // main_command (CREATE, GET, SET, etc)
	TOKEN_IDENTIFIER,   // identifier (name)
	TOKEN_STRING,       // string between quotes
	TOKEN_NUMBER,       // number
	TOKEN_OPERATOR,     // operator (=, -, etc)
	TOKEN_DELIMITER,    // delimiter (,, ;, etc)
	TOKEN_EOF,          // end of file
	TOKEN_ERROR         // lexical error
} token_type;

// structure of a token
typedef struct {
	token_type type;
	char *value;
	int position;
} token;

// structure of a parsed command
typedef struct {
	char *command;      // main command
	char **args;        // argument
	int arg_count;      // number of arguments
	int is_valid;       // validation flag
	char *error_msg;    // error_message
} parsed_command;

// structure of lexer
typedef struct {
	const char *input;
	size_t position;
	size_t length;
	char current_char;
} lexer;

// public functions
lexer *lexer_create(const char *input);
void lexer_free(lexer *lex);

token *lexer_next_token(lexer *lex);
void token_free(token *tok);

parsed_command *parse_command(const char *input);
void print_parsed_command(const parsed_command *cmd);
void free_parsed_command(parsed_command *cmd);


#endif // PARSER_H
