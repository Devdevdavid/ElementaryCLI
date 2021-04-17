#ifndef CLI_H
#define CLI_H

// ======================
// Includes
// ======================

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cli_config.h"
#include "line_buffer.h"

// ======================
// Constants
// ======================

#define CLI_NAME    "ElementaryCLI"
#define CLI_VERSION "0.1.4"

#define CLI_CMD_MAX_LEN     LB_LINE_BUFFER_LENGTH /**< Maximum length of a line */
#define CLI_ROOT_TOKEN_NAME "."                   /**< Name of the root token */

// ======================
// Typedefs and structs
// ======================

typedef int (*cli_callback_t)(uint8_t argc, char * argv[]); /**< Prototype of the function callable by cli commands */

typedef struct cli_token_t cli_token; /**< Needed because we have self pointer into this structure */
struct cli_token_t {
	char           text[CLI_MAX_TEXT_LEN]; /**< Name of the token */
	char           desc[CLI_MAX_DESC_LEN]; /**< Description of the token */
	cli_token *    childs[CLI_MAX_CHILDS]; /**< Pointer to all token child (None if leaf) */
	uint8_t        mandatoryArgc;          /**< Number of mandatory argument of the leaf */
	uint8_t        optionalArgc;           /**< Number of optional argument of the leaf */
	cli_callback_t callback;               /**< Function to call when user type the command */
	uint8_t        isLeaf : 1;             /**< Tell if token is a leaf */
};

// ======================
// Protoypes
// ======================

int          cli_init(void);
void         cli_strcpy_safe(char * dest, const char * src, uint16_t maxLen);
const char * cli_get_version(void);
cli_token *  cli_add_token(const char * text, const char * desc);
int          cli_add_children(cli_token * parent, cli_token * children);
int          cli_set_callback(cli_token * curTok, cli_callback_t callback);
int          cli_set_argc(cli_token * curTok, uint8_t mandatoryArgc, uint8_t optionalArgc);
cli_token *  cli_get_root_token(void);
uint8_t      cli_autocomplete_lb(const char * str, uint16_t len, char * outBuffer, uint16_t outBufferMaxLen);
int          cli_execute_lb(const char * str, uint16_t len);
void         cli_rx(uint8_t byte);
void         cli_exit(void);

#endif /* CLI_H */