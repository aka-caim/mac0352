#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

/* Parsed representation of a full command line.
 *
 * Stores the normalized command name, positional arguments, and validation
 * metadata used by consumers to decide whether execution can proceed.
 */
typedef struct {
	char *command;
	char **args;
	int arg_count;
	int is_valid; // nonzero means valid parse, zero means invalid parse
	char *error_msg; // human-readable error description if is_valid == 0
} parsed_command;

/* Parses a full input command and returns a structured representation.
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

/* Prints a human-readable representation of a parsed command.
 *
 * Intended for debugging, diagnostics, or quick inspection during development.
 * Output format depends on implementation details.
 *
 * Parameters:
 * - cmd: parsed command to be printed.
 */
void print_parsed_command(const parsed_command *cmd);

/* Releases all resources associated with a parsed_command object.
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
