#include "cli.h"

#define CLI_C
#include "debug.h"

// Global variables
const char * cliVersionName = CLI_NAME " - v" CLI_VERSION;
cli_token    tokenList[CLI_MAX_TOKEN_COUNT];

// ===================
//      TOOLS
// ===================

/**
 * @brief Print cmdText
 *
 * @param cmdText Array of pointer
 * @param count Number of element into the array
 */
static void cli_print_cmd_text(char * cmdText[], int count)
{
	DPRINTF(INFO, "Cmd text found (%d):\n\r", count);
	for (int i = 0; i < count; ++i) {
		printf("\t%s\n\r", cmdText[i]);
	}
}

/**
 * @brief Print all childs formatted with tree view
 * @warning Recurcive call inside !
 *
 * @param curTok The token where the tree begin
 */
static void cli_print_token_tree(cli_token * curTok)
{
	static int indent = 0;

	// Print indent
	for (int i = 0; i < indent; ++i) {
		printf(" | ");
	}
	++indent;

	// Print text and description with alignement
	printf("%s", curTok->text);
	for (int i = 0; i < (30 - 3 * indent - strlen(curTok->text)); ++i) {
		printf(" ");
	}
	printf("%s\n\r", curTok->desc);

	// Recursive call for all childs
	for (int i = 0; i < CLI_MAX_CHILDS; ++i) {
		if (curTok->childs[i] == NULL) {
			continue;
		}
		cli_print_token_tree(curTok->childs[i]);
	}

	--indent;
}

/**
 * @brief Print a token name with its description
 *
 * @param curTok Pointer
 */
static void cli_print_token(cli_token * curTok)
{
	printf("\t%s\t%s\n\r", curTok->text, curTok->desc);
}

// ===================
//      STATIC
// ===================

/**
 * @brief Give the usage for a specified token
 *
 * @param curTok Pointer
 */
static void usage(cli_token * curTok)
{
	// Not the same header if root
	printf("Usage");
	if (curTok == cli_get_root_token()) {
		printf(":\n\r");
	} else {
		printf(" for \"%s\":\n\r", curTok->text);
	}

	// Incase of leaf, we just print itself with its description
	if (cli_is_token_a_leaf(curTok)) {
		cli_print_token(curTok);
	} else {
		// Print all child descriptions
		for (int i = 0; i < CLI_MAX_CHILDS; ++i) {
			if (curTok->childs[i] == NULL) {
				continue;
			}
			cli_print_token(curTok->childs[i]);
		}
	}
}

/**
 * @brief Find the last valid token that match the command texts
 *
 * @param cmdText Array of char pointer : [0] -> "word1\0", [1] -> "word2\0",
 * etc.
 * @param cmdTextCount Number of element in cmdText
 * @param curTok Returned pointer
 * @return >= 0: depth (number of token in command), -1: text is unknown (last
 * known token returned)
 */
static int cli_find_last_valid_token(char * cmdText[], int cmdTextCount, cli_token ** curTok)
{
	int depth = 0;

	DPRINTF(FINDER, "- Entering\n\r");

	// Begin at root
	(*curTok) = cli_get_root_token();

	for (int i = 0; i < cmdTextCount; ++i) {
		int childIndex;

		// Search text into tokens
		for (childIndex = 0; childIndex < CLI_MAX_CHILDS; ++childIndex) {
			// Filter out empty childs
			if ((*curTok)->childs[childIndex] == NULL) {
				continue;
			}
			DPRINTF(FINDER, "Looking child: %s\n\r",
					(*curTok)->childs[childIndex]->text);

			if (strncmp((*curTok)->childs[childIndex]->text, cmdText[i],
						CLI_MAX_TEXT_LEN) == 0) {
				// Found it !
				(*curTok) = (*curTok)->childs[childIndex];
				++depth;
				DPRINTF(FINDER, "Identified child %s (%d)\n\r", (*curTok)->text,
						childIndex);
				break;
			}
		}

		// Check not found
		if (childIndex >= CLI_MAX_CHILDS) {
			DPRINTF(FINDER, "- failed\n\r");
			return -1;
		}

		// Check arguments - If this child has arguments, remaining cmdText should
		// be arguments
		if ((*curTok)->argc > 0) {
			DPRINTF(FINDER, "- Next must be arguments...\n\r");
			break;
		}
	}
	DPRINTF(FINDER, "- Leaving\n\r");

	return depth;
}

// ===================
//       EXTERN
// ===================

/**
 * @brief Safely copy src into dest with a max length and
 * set the ending string character
 * @details [long description]
 *
 * @param dest Pointer
 * @param src Pointer
 * @param maxLen Max length of dest
 */
void cli_strcpy_safe(char * dest, const char * src, int maxLen)
{
	strncpy(dest, src, maxLen);
	dest[maxLen - 1] = '\0';
}

/**
 * @brief Tell if given token is a leaf
 * @details A leaf is a token with no children
 *
 * @param curTok Pointer
 * @return boolean
 */
bool cli_is_token_a_leaf(cli_token * curTok)
{
	return curTok->isLeaf == true;
}

/**
 * @brief Add a token to the list of known tokens
 *
 * @param text The text of the token
 * @param desc The description of the token
 * @return a pointer to the newly created token
 */
cli_token * cli_add_token(const char * text, const char * desc)
{
	cli_token * curTok = NULL;

	// Find a not used token (Considered not used if text is not set)
	for (int i = 0; i < CLI_MAX_TOKEN_COUNT; ++i) {
		if (tokenList[i].text[0] == '\0') {
			curTok = &tokenList[i];
			break;
		}
	}

	// Check overflow
	if (curTok == NULL) {
		DPRINTF(ERROR, "Unable to add token \"%s\", maximum reach: %d\n\r",
				curTok->text, CLI_MAX_TOKEN_COUNT);
		return NULL;
	}

	// Clear and fill the structure
	memset(curTok, 0, sizeof(*curTok));
	cli_strcpy_safe(curTok->text, text, CLI_MAX_TEXT_LEN);
	cli_strcpy_safe(curTok->desc, desc, CLI_MAX_DESC_LEN);
	curTok->isLeaf = true;

	return curTok;
}

/**
 * @brief Add a children to a token
 *
 * @param parent Pointer
 * @param children Pointer
 *
 * @return 0: ok, -1: Can't take more children
 */
int cli_add_children(cli_token * parent, cli_token * children)
{
	if (parent->argc > 0) {
		DPRINTF(
		ERROR,
		"Unable to add children for token \"%s\", parent has %d arguments\n\r",
		parent->text, parent->argc);
		return -1;
	}

	for (int i = 0; i < CLI_MAX_CHILDS; ++i) {
		if (parent->childs[i] == NULL) {
			parent->childs[i] = children;

			// Token with children are no longer a leaf
			parent->isLeaf = false;
			return 0;
		}
	}

	DPRINTF(ERROR,
			"Unable to add children for token \"%s\", parent has no empty child "
			"(CLI_MAX_CHILDS = %d)\n\r",
			parent->text, CLI_MAX_CHILDS);
	return -1;
}

/**
 * @brief Set callback
 * @details [long description]
 *
 * @param curTok Pointer
 * @param callback Pointer to function
 * @return 0
 */
int cli_set_callback(cli_token * curTok, cli_callback_t callback)
{
	if (!cli_is_token_a_leaf(curTok)) {
		DPRINTF(ERROR,
				"Unable to set callback for token \"%s\", token is not a leaf\n\r",
				curTok->text);
		return -1;
	}
	curTok->callback = callback;
	return 0;
}

/**
 * @brief Get the children count of a token
 *
 * @param parent Pointer
 * @return Number of children [0; CLI_MAX_CHILDS]
 */
int cli_get_children_count(cli_token * parent)
{
	int count = 0;

	for (int i = 0; i < CLI_MAX_CHILDS; ++i) {
		if (parent->childs[i] != NULL) {
			++count;
		}
	}
	return count;
}

/**
 * @brief Set the argument count for this token
 *
 * @param curTok Pointer
 * @param argc int
 *
 * @return 0: ok, -1: Error
 */
int cli_set_argc(cli_token * curTok, int argc)
{
	if (!cli_is_token_a_leaf(curTok)) {
		DPRINTF(
		ERROR,
		"Can't set argument count for token \"%s\": token is not a leaf\n\r",
		curTok->text);
		return -1;
	}

	if (argc <= 0) {
		DPRINTF(ERROR, "Invalid argument count for token \"%s\": %d\n\r",
				curTok->text, argc);
		return -1;
	}

	curTok->argc = argc;
	return 0;
}

/**
 * @brief Give the root token pointer
 * @return Pointer to root token
 */
cli_token * cli_get_root_token(void)
{
	return &tokenList[0];
}

/**
 * @brief Init module
 * @return 0
 */
int cli_init(void)
{
	DEBUG_ENABLE(ERROR);
	//DEBUG_ENABLE(PARSER);
	//DEBUG_ENABLE(FINDER);

	// Add root children
	cli_add_token(CLI_ROOT_TOKEN_NAME, "");

	// Init LineBuffer
	lb_init();
	lb_set_valid_line_callback(&cli_input_command);
	return 0;
}

/**
 * @brief Execute a command
 *
 * @param cmdText Array of char pointer : [0] -> "word1\0", [1] -> "word2\0",
 * etc.
 * @param cmdTextCount Number of element in cmdText
 * @return The callback return, -1: Error
 */
int cli_execute_command(char * cmdText[], int cmdTextCount)
{
	cli_token * curTok = cli_get_root_token();
	int         depth;
	char **     argv = NULL;

	// FIND TOKENS
	depth = cli_find_last_valid_token(cmdText, cmdTextCount, &curTok);
	if (depth <= 0) {
		usage(curTok);
		return -1;
	}

	// Error if not a leaf
	if (cli_is_token_a_leaf(curTok) == false) {
		usage(curTok);
		return -1;
	}

	// Check arguments
	if (curTok->argc > 0) {
		// Get the number of cmdText that are not tokens
		if ((cmdTextCount - depth) < curTok->argc) {
			DPRINTF(ERROR, "This command takes %d arguments !\n\r", curTok->argc);
			usage(curTok);
			return -1;
		}

		// The first argument starts right after the last valid token
		argv = &cmdText[depth];
	}

	// Check null callback
	if (curTok->callback == NULL) {
		DPRINTF(ERROR, "No callback defined for this command !\n\r");
		return -1;
	}

	// Call the function eventually and return its value
	return curTok->callback(curTok->argc, argv);
}

/**
 * @brief Auto-complete a command or propose choice
 *
 * @param cmdText Array of char pointer : [0] -> "word1\0", [1] -> "word2\0",
 * etc.
 * @param cmdTextCount Number of element in cmdText
 * @return NULL: Either no alternative or more than one, >0: A pointer to the
 * only alternative possible
 */
char * cli_auto_complete(char * cmdText[], int cmdTextCount)
{
	cli_token * curTok             = cli_get_root_token();
	cli_token * lastAlternativeTok = NULL;
	int         lastCmdTextLen;
	char *      lastCmdText;
	int         alternatives = 0;

	// FIND TOKENS
	// if it failed, propose alternatives
	if (cli_find_last_valid_token(cmdText, cmdTextCount, &curTok) != 0) {
		// Shortcuts
		lastCmdText    = cmdText[cmdTextCount - 1];
		lastCmdTextLen = strlen(lastCmdText);

		// Do this in 2 states:
		// 1. Count alternatives
		// 2. Print them if more than 1
		for (int state = 0; state < 2; ++state) {
			// For all childs of the last valid token found...
			for (int i = 0; i < CLI_MAX_CHILDS; ++i) {
				if (curTok->childs[i] == NULL) {
					continue;
				}
				// Does the last text match this child ?
				if (strncmp(lastCmdText, curTok->childs[i]->text, lastCmdTextLen) ==
					0) {
					if (state == 0) {
						++alternatives;
						lastAlternativeTok = curTok->childs[i];
					} else if (state == 1) {
						printf("%s\t%s\n\r", curTok->childs[i]->text,
							   curTok->childs[i]->desc);
					}
				}
			}

			if (alternatives == 0) {
				break; // Nothing to do
			} else if (alternatives == 1) {
				return lastAlternativeTok->text;
			} else {
				continue; // Will print alternatives
			}
		}
	}

	return NULL;
}

/**
 * @brief Give a string containing the version of CLI
 * @return String pointer
 */
const char * cli_get_version(void)
{
	return cliVersionName;
}

/**
 * @brief Split a string by spaces into a list of strings
 *
 * @param cmdEdit Editable buffer "word1 word2 word3"
 * @param cmdText Array of char pointer : [0] -> "word1\0", [1] -> "word2\0",
 * etc.
 *
 * @return Number of cmdText found (Ex: 3), -1: Error
 */
int cli_parse_cmd_text(char * cmdEdit, char * cmdText[])
{
	int    cmdTextCount = 0;
	char * pCmd         = cmdEdit;

	DPRINTF(PARSER, "- Entering\n\r");

	if ((*pCmd) == '\0') {
		DPRINTF(PARSER, "- Nothing to parse\n\r");
		return 0; // Empty string
	}

	cmdText[0] = pCmd;

	while (1) {
		if ((*pCmd) == '\0') {
			// To get count instead of index
			++cmdTextCount;
			break;
		} else if ((*pCmd) == ' ') {
			// All spaces must be replaced by ending string
			*pCmd = '\0';
			++pCmd;

			// A new cmdText begins only if next character is not a space
			// This allow user to put multiple spaces between cmdText
			if ((*pCmd) != ' ') {
				++cmdTextCount;
				cmdText[cmdTextCount] = pCmd;

				// Check maximum
				if (cmdTextCount >= CLI_CMD_MAX_TOKEN) {
					DPRINTF(ERROR, "Limit is reached (CLI_CMD_MAX_TOKEN = %d)\n\r", CLI_CMD_MAX_TOKEN);
					return -1;
				}
			}
		} else {
			++pCmd;
		}
	}

	DEBUG_BLOC(PARSER)
	{
		cli_print_cmd_text(cmdText, cmdTextCount);
		DPRINTF(PARSER, "- Leaving\n\r");
	}

	return cmdTextCount;
}

/**
 * @brief Callback function for LineBuffer
 * @details The definition of this function must follow
 * the one defined in LineBuffer
 * @see lb_line_callback_t
 *
 * @param str The input command string
 * @param len The length of the str
 *
 * @return The result of the command
 */
int cli_input_command(const char * str, int len)
{
	char   cmdEdit[CLI_CMD_MAX_LEN]; // Editable copy of str
	char * cmdText[CLI_CMD_MAX_TOKEN];
	int    cmdTextCount;

	// Copy incomming buffer
	cli_strcpy_safe(cmdEdit, str, CLI_CMD_MAX_LEN);

	// PARSER
	cmdTextCount = cli_parse_cmd_text(cmdEdit, cmdText);
	if (cmdTextCount <= 0) {
		return 0;
	}

	// Search for alternatives
	// char * pText = cli_auto_complete(cmdText, cmdTextCount);
	// if (pText != NULL) {
	// 	printf("Alternative: %s\n\r", pText);
	// }

	return cli_execute_command(cmdText, cmdTextCount);
}

/**
 * @brief Input of caracter to manage by cli
 *
 * @param byte The value of the caracter
 */
void cli_rx(uint8_t byte)
{
	lb_rx(byte);
}