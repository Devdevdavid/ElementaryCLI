#ifndef CLI_CONFIG_H
#define CLI_CONFIG_H

/* EXTERN USER FUNCTIONS */

/* CLI */
#define CLI_MAX_CHILDS      4  /**< Maximum number of childs for a token */
#define CLI_MAX_TEXT_LEN    10 /**< Maximum length of the token's text attribute */
#define CLI_MAX_DESC_LEN    32 /**< Maximum length og the token's description attribute */
#define CLI_MAX_TOKEN_COUNT 10 /**< Maximum number of tokens */
#define CLI_CMD_MAX_TOKEN   5  /**< Maximum number of cmdText in a line (including tokens and arguments) */

#define CLI_PRINTF(...) printf(__VA_ARGS__); /**< Standard output */

/* LINE BUFFER */
#define LB_LINE_BUFFER_LENGTH 32 /**< Maximum number of character into the line buffer */
#define LB_HISTORY_COUNT      10 /**< Maximum number of line in history */

#endif /* CLI_CONFIG_H */