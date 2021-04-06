#ifndef CLI_H
#define CLI_H

// ======================
// Includes
// ======================

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "lineBuffer.h"

// ======================
// Constants
// ======================

#define CLI_NAME    "ElementaryCLI"
#define CLI_VERSION "0.1.0"

#define CLI_MAX_CHILDS      4
#define CLI_MAX_TEXT_LEN    10
#define CLI_MAX_DESC_LEN    32
#define CLI_MAX_TOKEN_COUNT 10
#define CLI_CMD_MAX_TOKEN   5
#define CLI_CMD_MAX_LEN     LB_LINE_BUFFER_LENGTH
#define CLI_ROOT_TOKEN_NAME "."

// ======================
// Typedefs and structs
// ======================

typedef int (*cli_callback_t)(int argc, char * argv[]); /**< Prototype of the function callable by cli commands */

typedef struct cli_token_t cli_token; /**< Needed because we have self pointer into this structure */
struct cli_token_t {
	char           text[CLI_MAX_TEXT_LEN]; /**< Name of the token */
	char           desc[CLI_MAX_DESC_LEN]; /**< Description of the token */
	cli_token *    childs[CLI_MAX_CHILDS]; /**< Pointer to all token child (None if leaf) */
	int            argc;                   /**< Number of arguments required by the leaf */
	cli_callback_t callback;               /**< Function to call when user type the command */
	uint8_t        isLeaf : 1;             /**< Tell if token is a leaf */
};

// ======================
// Protoypes
// ======================

void         cli_strcpy_safe(char * dest, const char * src, int maxLen);
bool         cli_is_token_a_leaf(cli_token * curTok);
cli_token *  cli_add_token(const char * text, const char * desc);
int          cli_add_children(cli_token * parent, cli_token * children);
int          cli_set_callback(cli_token * curTok, cli_callback_t callback);
int          cli_get_children_count(cli_token * parent);
int          cli_set_argc(cli_token * curTok, int argc);
cli_token *  cli_get_root_token(void);
int          cli_init(void);
int          cli_execute_command(char * cmdText[], int cmdTextCount);
char *       cli_auto_complete(char * cmdText[], int cmdTextCount);
const char * cli_get_version(void);
int          cli_parse_cmd_text(char * cmdEdit, char * cmdText[]);
int          cli_autocomplete_command(const char * str, int len, char * outBuffer, int outBufferMaxLen);
int          cli_input_command(const char * str, int len);
void         cli_rx(uint8_t byte);

#endif /* CLI_H */