#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

static char lexer_peek(lexer *lex, int offset) {
    size_t peek_pos = lex->position + offset;

	if (peek_pos >= lex->length)
        return '\0';

    return lex->input[peek_pos];
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
    
    while (lex->current_char != '\0' && lex->current_char != quote && idx < 1023) {
        if (lex->current_char == '\\') {
            lexer_advance(lex);
            // Escapar caracteres especiais
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
    
    return token_create(TOKEN_ERROR, "String não terminada", start_pos);
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
    
    // known commands (converted to lower case)
    char upper_buffer[256];
    for (int i = 0; buffer[i]; i++)
        upper_buffer[i] = toupper(buffer[i]);
    upper_buffer[idx] = '\0';
    
    // check if it's a command (first word)
    if (start_pos == 0 || 
        (start_pos > 0 && isspace(lex->input[start_pos - 1]))) {
        if (strcmp(upper_buffer, "GET") == 0 ||
            strcmp(upper_buffer, "SET") == 0 ||
            strcmp(upper_buffer, "DELETE") == 0 ||
            strcmp(upper_buffer, "LIST") == 0 ||
            strcmp(upper_buffer, "HELP") == 0 ||
            strcmp(upper_buffer, "QUIT") == 0 ||
            strcmp(upper_buffer, "EXIT") == 0) {
            return token_create(TOKEN_COMMAND, upper_buffer, start_pos);
        }
    }
    
    return token_create(TOKEN_IDENTIFIER, buffer, start_pos);
}

/* public methods */

void lexer_free(lexer *lex) {
    if (lex)
		free(lex);
}

lexer *lexer_create(const char *input) {
    if (!input) return NULL;
    
    lexer *lex = malloc(sizeof(*lex));
    if (!lex) return NULL;
    
    lex->input = input;
    lex->position = 0;
    lex->length = strlen(input);
    lex->current_char = lex->length > 0 ? input[0] : '\0';
    
    return lex;
}

void token_free(token *tok) {
    if (tok) {
        free(tok->value);
        free(tok);
    }
}

token* lexer_next_token(lexer *lex) {
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
            return token_create(TOKEN_ERROR, "Caractere inválido", current_pos);
    }
}

parsed_command* parse_command(const char *input) {
    if (!input || strlen(input) == 0) {
        parsed_command *cmd = malloc(sizeof(*cmd));
        if (cmd) {
            cmd->command = NULL;
            cmd->args = NULL;
            cmd->arg_count = 0;
            cmd->is_valid = 0;
            cmd->error_msg = strdup_safe("Entrada vazia");
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
        cmd->error_msg = strdup_safe("Erro ao criar lexer");
        return cmd;
    }
    
    // read first token (must be a command)
    token *tok = lexer_next_token(lex);
    
    if (!tok) {
        cmd->error_msg = strdup_safe("Erro ao ler token");
        lexer_free(lex);
        return cmd;
    }
    
    if (tok->type != TOKEN_COMMAND) {
        cmd->error_msg = strdup_safe("Comando inválido");
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
    
    if (!cmd->error_msg) {
        cmd->is_valid = 1;
    }
    
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
        printf("Comando NULL\n");
        return;
    }
    
    printf("Parsed command\n");
    printf("Valid? %s\n", cmd->is_valid ? "Yes" : "No");
    
    if (cmd->error_msg)
        printf("Erro: %s\n", cmd->error_msg);
    
    if (cmd->command)
        printf("Comando: %s\n", cmd->command);
    
    printf("Argumentos (%d):\n", cmd->arg_count);
    for (int i = 0; i < cmd->arg_count; i++)
        printf("  [%d]: %s\n", i, cmd->args[i]);
}
