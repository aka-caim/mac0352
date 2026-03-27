#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

/*
 * Token categories produced by the lexical analyzer.
 *
 * These values describe the syntactic role of each lexeme extracted from
 * the input stream and are used by the parser to validate command grammar.
 */
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

/*
 * Lexical unit emitted by the tokenizer.
 *
 * Each token captures its classification, textual representation, and source
 * position to support parsing, diagnostics, and error reporting.
 */
typedef struct {
	/* Category of this token. */
	token_type type;

	/*
	 * Token text.
	 *
	 * Conventionally points to dynamically allocated memory owned by the token
	 * object and released by token_free().
	 */
	char *value;

	/*
	 * Zero-based character index in the original input where the token starts.
	 */
	int position;
} token;

/*
 * Parsed representation of a full command line.
 *
 * Stores the normalized command name, positional arguments, and validation
 * metadata used by consumers to decide whether execution can proceed.
 */
typedef struct {
	/* Main command keyword identified in the input. */
	char *command;

	/* Dynamic array of argument strings in parse order. */
	char **args;

	/* Number of valid entries currently stored in args. */
	int arg_count;

	/*
	 * Parse status flag.
	 * Typical convention: nonzero means valid parse, zero means invalid parse.
	 */
	int is_valid;

	/*
	 * Human-readable error description when is_valid indicates failure.
	 * May be NULL when parsing succeeds.
	 */
	char *error_msg;
} parsed_command;

/*
 * Stateful lexer cursor over an input string.
 *
 * Holds scan progress and current character so token extraction can be done
 * incrementally across repeated calls to lexer_next_token().
 */
typedef struct {
	/* Source text being tokenized (not owned by lexer). */
	const char *input;

	/* Current zero-based cursor position in input. */
	size_t position;

	/* Cached total length of input. */
	size_t length;

	/* Character currently pointed to by position (implementation-dependent). */
	char current_char;
} lexer;

// public functions

/*
 * Creates and initializes a lexer instance for the provided input string.
 *
 * The lexer keeps a pointer to the original input text and iterates through it
 * character by character when tokenization functions are called.
 *
 * Parameters:
 * - input: null-terminated string to be analyzed.
 *
 * Returns:
 * - Pointer to a newly allocated lexer on success.
 * - NULL if allocation fails or if input is invalid.
 *
 * Ownership:
 * - The caller owns the returned lexer and must release it with lexer_free().
 * - The input string must remain valid while the lexer is in use.
 */
lexer *lexer_create(const char *input);

/*
 * Releases all resources associated with a lexer instance.
 *
 * This function only frees memory owned by the lexer object itself.
 * It does not free the input string originally passed to lexer_create().
 *
 * Parameters:
 * - lex: lexer pointer to free (accepts NULL, if implementation supports it).
 */
void lexer_free(lexer *lex);

/*
 * Reads and returns the next token from the lexer stream.
 *
 * Each call advances the internal lexer cursor and extracts one token based on
 * lexical rules (identifiers, strings, numbers, operators, delimiters, etc).
 *
 * Parameters:
 * - lex: initialized lexer instance.
 *
 * Returns:
 * - Pointer to a newly allocated token describing the next lexical element.
 * - TOKEN_EOF when end of input is reached.
 * - TOKEN_ERROR in case of lexical inconsistencies.
 * - NULL if allocation fails or if lex is invalid.
 *
 * Ownership:
 * - The caller owns the returned token and must release it with token_free().
 */
token *lexer_next_token(lexer *lex);

/*
 * Releases memory associated with a token object.
 *
 * Typically frees both the token structure and any dynamic memory referenced
 * by token fields (for example, token value string).
 *
 * Parameters:
 * - tok: token pointer to free (accepts NULL, if implementation supports it).
 */
void token_free(token *tok);

/*
 * Parses a full input command and returns a structured representation.
 *
 * This is the high-level API that combines lexical analysis and syntactic
 * validation to produce a parsed_command object with command name, argument
 * vector, argument count, and validation/error metadata.
 *
 * Parameters:
 * - input: null-terminated command string to parse.
 *
 * Returns:
 * - Pointer to a newly allocated parsed_command containing the parse result.
 * - The result may indicate success or failure through is_valid and error_msg.
 * - NULL if allocation fails or if input is invalid.
 *
 * Ownership:
 * - The caller owns the returned parsed_command and must release it with
 *   free_parsed_command().
 */
parsed_command *parse_command(const char *input);

/*
 * Prints a human-readable representation of a parsed command.
 *
 * Intended for debugging, diagnostics, or quick inspection during development.
 * Output format depends on implementation details.
 *
 * Parameters:
 * - cmd: parsed command to be printed.
 */
void print_parsed_command(const parsed_command *cmd);

/*
 * Releases all resources associated with a parsed_command object.
 *
 * Frees command string, argument vector, each argument element (if dynamically
 * allocated), error message, and the parsed_command structure itself.
 *
 * Parameters:
 * - cmd: parsed command pointer to free (accepts NULL, if implementation
 *   supports it).
 */
void free_parsed_command(parsed_command *cmd);


#endif // PARSER_H
