#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* private data structures */

typedef enum {
	TOKEN_COMMAND,      /* command keyword (CREATE, GET, SET, etc) */
	TOKEN_IDENTIFIER,   /* symbolic name (identifier) */
	TOKEN_STRING,       /* quoted string literal */
	TOKEN_NUMBER,       /* numeric literal */
	TOKEN_OPERATOR,     /* operator symbol (=, -, etc) */
	TOKEN_DELIMITER,    /* punctuation separator (,, ;, etc) */
	TOKEN_EOF,          /* logical end of input */
	TOKEN_ERROR         /* invalid or unrecognized lexeme */
} token_type;

/* Lexical unit emitted by the tokenizer.
 *
 * Each token captures its classification, textual representation, and source
 * position to support parsing, diagnostics, and error reporting.
 */
typedef struct {
	token_type type;
	char *value; // token text
	int position; // zero-based char index in the original input
} token;

/* Stateful lexer cursor over an input string.
 *
 * Holds scan progress and current character so token extraction can be done
 * incrementally across repeated calls to lexer_next_token().
 */
typedef struct {
	const char *input;
	size_t position; // current zero-based cursor position in input
	size_t length; // cached total length of input
	char current_char; // char currenctly pointed to by position
} lexer;


/* private, helper methods*/

static char* strdup_safe(const char *str) {
    if (!str) return NULL;
    char *dup = malloc(strlen(str) + 1);
    if (dup) strcpy(dup, str);
    return dup;
}

static void lexer_advance(lexer *lex) {
    lex->position++;

    if (lex->position >= lex->length)
        lex->current_char = '\0';
    else
        lex->current_char = lex->input[lex->position];
}

static void lexer_skip_whitespace(lexer *lex) {
    while (lex->current_char != '\0' && isspace(lex->current_char))
        lexer_advance(lex);
}

static token* token_create(token_type type, const char *value, int position) {
    token *tok = malloc(sizeof(*tok));
    if (!tok) return NULL;
    
    tok->type = type;
    tok->value = strdup_safe(value);
    tok->position = position;
    
    return tok;
}

static token* lexer_read_string(lexer *lex) {
    int start_pos = lex->position;
    char quote = lex->current_char;
    lexer_advance(lex); // ignore opening quotes
    
    char buffer[1024];
    int idx = 0;
    
    while (lex->current_char != '\0' &&
				lex->current_char != quote && idx < 1023) {
        if (lex->current_char == '\\') {
            lexer_advance(lex);

            switch (lex->current_char) {
                case 'n': buffer[idx++] = '\n'; break;
                case 't': buffer[idx++] = '\t'; break;
                case '\\': buffer[idx++] = '\\'; break;
                case '"': buffer[idx++] = '"'; break;
                case '\'': buffer[idx++] = '\''; break;
                default: buffer[idx++] = lex->current_char;
            }
        } else
            buffer[idx++] = lex->current_char;
        lexer_advance(lex);
    }
    
    buffer[idx] = '\0';
    
    if (lex->current_char == quote) {
        lexer_advance(lex); // ignore closing quotes
        return token_create(TOKEN_STRING, buffer, start_pos);
    }
    
    return token_create(TOKEN_ERROR, "Unfinished string", start_pos);
}

static token* lexer_read_number(lexer *lex) {
    int start_pos = lex->position;
    char buffer[256];
    int idx = 0;
    
    while (lex->current_char != '\0' && 
           (isdigit(lex->current_char) || lex->current_char == '.') && 
			   idx < 255) {
        buffer[idx++] = lex->current_char;
        lexer_advance(lex);
    }
    
    buffer[idx] = '\0';
    return token_create(TOKEN_NUMBER, buffer, start_pos);
}

static token* lexer_read_identifier(lexer *lex) {
    int start_pos = lex->position;
    char buffer[256];
    int idx = 0;
    
    while (lex->current_char != '\0' && 
           (isalnum(lex->current_char) || lex->current_char == '_') && 
			   idx < 255) {
        buffer[idx++] = lex->current_char;
        lexer_advance(lex);
    }
    
    buffer[idx] = '\0';
    
    // known commands (converted to upper case)
    char upper_buffer[256];
    for (int i = 0; buffer[i]; i++)
        upper_buffer[i] = toupper(buffer[i]);
    upper_buffer[idx] = '\0';
    
    // check if it's a command (first word)
    if (start_pos == 0 || 
        (start_pos > 0 && isspace(lex->input[start_pos - 1]))) {
        /*
         * Accepted command keywords.
         * Unrecognized keywords remain TOKEN_IDENTIFIER so parse_command()
         * can reject them as invalid commands.
         */
        if (strcmp(upper_buffer, "CREATE") == 0 ||
            strcmp(upper_buffer, "GET") == 0 ||
            strcmp(upper_buffer, "SET") == 0 ||
            strcmp(upper_buffer, "RESERVE") == 0 ||
            strcmp(upper_buffer, "RELEASE") == 0 ||
            strcmp(upper_buffer, "LIST") == 0) {
            return token_create(TOKEN_COMMAND, upper_buffer, start_pos);
        }
    }
    
    return token_create(TOKEN_IDENTIFIER, buffer, start_pos);
}

static void lexer_free(lexer *lex) {
    if (lex)
		free(lex);
}

static lexer *lexer_create(const char *input) {
    if (!input) return NULL;
    
    lexer *lex = malloc(sizeof(*lex));
    if (!lex) return NULL;
    
    lex->input = input;
    lex->position = 0;
    lex->length = strlen(input);
    lex->current_char = lex->length > 0 ? input[0] : '\0';
    
    return lex;
}

static void token_free(token *tok) {
    if (tok) {
        free(tok->value);
        free(tok);
    }
}

static token* lexer_next_token(lexer *lex) {
    if (!lex) return NULL;
    
    lexer_skip_whitespace(lex);
    
    if (lex->current_char == '\0')
        return token_create(TOKEN_EOF, "", lex->position);
    
    int current_pos = lex->position;
    
    // string between quotes
    if (lex->current_char == '"' || lex->current_char == '\'')
        return lexer_read_string(lex);

    if (isdigit(lex->current_char))
        return lexer_read_number(lex);
    
    // identifier or command
    if (isalpha(lex->current_char) || lex->current_char == '_')
        return lexer_read_identifier(lex);
    
    // operators and delimiters
    char op[2] = {lex->current_char, '\0'};
    lexer_advance(lex);
    
    switch (op[0]) {
        case '=':
        case '+':
        case '-':
        case '*':
        case '/':
            return token_create(TOKEN_OPERATOR, op, current_pos);
        case ',':
        case ';':
            return token_create(TOKEN_DELIMITER, op, current_pos);
        default:
            return token_create(TOKEN_ERROR, "invalid char", current_pos);
    }
}

/* public methods */

parsed_command* parse_command(const char *input) {
    if (!input || strlen(input) == 0) {
        parsed_command *cmd = malloc(sizeof(*cmd));
        if (cmd) {
            cmd->command = NULL;
            cmd->args = NULL;
            cmd->arg_count = 0;
            cmd->is_valid = 0;
            cmd->error_msg = strdup_safe("Error: empty input");
        }
        return cmd;
    }
    
    parsed_command *cmd = malloc(sizeof(*cmd));
    if (!cmd) return NULL;
    
    cmd->command = NULL;
    cmd->args = NULL;
    cmd->arg_count = 0;
    cmd->is_valid = 0;
    cmd->error_msg = NULL;
    
    lexer *lex = lexer_create(input);
    if (!lex) {
        cmd->error_msg = strdup_safe("Error: could not create lexer");
        return cmd;
    }
    
    // read first token (must be a command)
    token *tok = lexer_next_token(lex);
    
    if (!tok) {
        cmd->error_msg = strdup_safe("Error: could not read token");
        lexer_free(lex);
        return cmd;
    }
    
    if (tok->type != TOKEN_COMMAND) {
        cmd->error_msg = strdup_safe("Invalid command");
        token_free(tok);
        lexer_free(lex);
        return cmd;
    }
    
    cmd->command = strdup_safe(tok->value);
    token_free(tok);
    
    // read arguments
    int capacity = 10;
    cmd->args = malloc(sizeof(char*) * capacity);
    
    while (1) {
        tok = lexer_next_token(lex);
        if (!tok || tok->type == TOKEN_EOF) {
            if (tok) token_free(tok);
            break;
        }
        
        if (tok->type == TOKEN_ERROR) {
            cmd->error_msg = strdup_safe(tok->value);
            token_free(tok);
            break;
        }
        
        if (tok->type == TOKEN_DELIMITER) {
            token_free(tok);
            continue; // ignore delimiters
        }
        
        // expand array if necessary
        if (cmd->arg_count >= capacity) {
            capacity *= 2;
            cmd->args = realloc(cmd->args, sizeof(char *) * capacity);
        }
        
        cmd->args[cmd->arg_count++] = strdup_safe(tok->value);
        token_free(tok);
    }
    
    lexer_free(lex);
    
    if (!cmd->error_msg)
        cmd->is_valid = 1;
    
    return cmd;
}

void free_parsed_command(parsed_command *cmd) {
    if (!cmd) return;
    
    free(cmd->command);
    free(cmd->error_msg);
    
    if (cmd->args) {
        for (int i = 0; i < cmd->arg_count; i++)
			free(cmd->args[i]);
        
		free(cmd->args);
    }
    
    free(cmd);
}

void print_parsed_command(const parsed_command *cmd) {
    if (!cmd) {
        printf("NULL command\n");
        return;
    }

	printf("Parsed message:\n");
    
    if (cmd->error_msg)
        printf("Error: %s\n", cmd->error_msg);
    
    if (cmd->command)
        printf("Command: %s\n", cmd->command);
    
    printf("Arguments (%d):\n", cmd->arg_count);
    for (int i = 0; i < cmd->arg_count; i++)
        printf("  [%d]: %s\n", i, cmd->args[i]);
}
