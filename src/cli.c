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
 * @brief Tell if given token is a leaf
 * @details A leaf is a token with no children
 *
 * @param curTok Pointer
 * @return boolean
 */
static bool cli_is_token_a_leaf(cli_token * curTok)
{
	return curTok->isLeaf == true;
}

/**
 * @brief Give the usage for a specified token
 *
 * @param curTok Pointer
 */
static void cli_usage(cli_token * curTok)
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
 * @return depth abs(depth): Number of valid tokens, <0: token abs(depth) + 1 is not valid
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
			DPRINTF(FINDER, "Looking child: %s\n\r", (*curTok)->childs[childIndex]->text);

			if (strncmp((*curTok)->childs[childIndex]->text, cmdText[i], CLI_MAX_TEXT_LEN) == 0) {
				// Found it !
				(*curTok) = (*curTok)->childs[childIndex];
				++depth;
				DPRINTF(FINDER, "Identified child %s (%d)\n\r", (*curTok)->text, childIndex);
				break;
			}
		}

		// Check not found
		if (childIndex >= CLI_MAX_CHILDS) {
			DPRINTF(FINDER, "- failed\n\r");
			return -depth; // Negative depth: depth first tokens are valid but not (depth+1)
		}

		// Check arguments - If this child has arguments, remaining cmdText should
		// be arguments
		if (((*curTok)->mandatoryArgc + (*curTok)->optionalArgc) > 0) {
			DPRINTF(FINDER, "- Next must be arguments...\n\r");
			break;
		}
	}
	DPRINTF(FINDER, "- Leaving\n\r");

	return depth;
}

/**
 * @brief Get the children count of a token
 *
 * @param parent Pointer
 * @return Number of children [0; CLI_MAX_CHILDS]
 */
static int cli_get_children_count(cli_token * parent)
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
 * @brief Split a string by spaces into a list of strings
 *
 * @param cmdEdit Editable buffer "word1 word2 word3"
 * @param cmdText Array of char pointer : [0] -> "word1\0", [1] -> "word2\0",
 * etc.
 *
 * @return Number of cmdText found (Ex: 3)
 */
static int cli_parse_cmd_text(char * cmdEdit, char * cmdText[])
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
			// and to an ending line
			// This allow user to put multiple spaces between cmdText and extra space
			// at the end of the line
			if (((*pCmd) != ' ') && (*pCmd) != '\0') {
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
 * @brief Execute a command
 *
 * @param cmdText Array of char pointer : [0] -> "word1\0", [1] -> "word2\0",
 * etc.
 * @param cmdTextCount Number of element in cmdText
 * @return The callback return, -1: Error
 */
static int cli_execute(char * cmdText[], int cmdTextCount)
{
	cli_token * curTok = cli_get_root_token();
	int         depth;
	char **     argv = NULL;
	int         argc; // Number of argument given by user

	// FIND TOKENS
	depth = cli_find_last_valid_token(cmdText, cmdTextCount, &curTok);
	if (depth <= 0) {
		// -depth is the index of the first not valid token
		// (+1 to get not valid, -1: because starts at 0)
		DPRINTF(ERROR, "Unknown command \"%s\"\n\r", cmdText[-depth]);
		goto retFailed;
	}

	// Error if not a leaf
	if (cli_is_token_a_leaf(curTok) == false) {
		goto retFailed;
	}

	// Get the number of cmdText that are not tokens
	// Compare this number to the number of mandatory arguments
	// If there is more, they are considered as optional arguments
	argc = cmdTextCount - depth;
	if (argc < curTok->mandatoryArgc) {
		DPRINTF(ERROR, "This command takes %d mandatory argument !\n\r", curTok->mandatoryArgc);
		goto retFailed;
	}

	if (argc > (curTok->mandatoryArgc + curTok->optionalArgc)) {
		DPRINTF(ERROR, "This command takes only %d optional argument !\n\r", curTok->optionalArgc);
		goto retFailed;
	}

	// The first argument starts right after the last valid token
	argv = &cmdText[depth];

	// Check null callback
	if (curTok->callback == NULL) {
		DPRINTF(ERROR, "No callback defined for this command !\n\r");
		return -1;
	}

	// Call the function eventually and return its value
	return curTok->callback(argc, argv);

	// Show usage and return error
retFailed:
	cli_usage(curTok);
	return -1;
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
static char * cli_autocomplete(char * cmdText[], int cmdTextCount)
{
	cli_token * curTok             = cli_get_root_token();
	cli_token * lastAlternativeTok = NULL;
	int         lastCmdTextLen;
	char *      lastCmdText;
	int         alternatives   = 0;
	char        emptyString[1] = "";
	int         depth;

	// FIND TOKENS
	// If all cmdText are recognized, there is no alternatives to give
	// If depth == 0, the last recognized is root, we must propose alternatives
	depth = cli_find_last_valid_token(cmdText, cmdTextCount, &curTok);
	if (depth > 0) {
		// There is no alternatives for leafs (We won't propose to complete arguments)
		// So just print usage
		if (cli_is_token_a_leaf(curTok)) {
			cli_usage(curTok);
			DPRINTF(AUTOC, "Last token is a leaf\n\r");
			return NULL;
		}
	}

	// Shortcuts
	if (cmdTextCount == depth) {
		// Here, user tries to autocomplete a already completed token
		// He actually wants all childs of this token
		// Use empty string to match all childs
		lastCmdText    = emptyString;
		lastCmdTextLen = 0;
	} else {
		lastCmdText    = cmdText[cmdTextCount - 1];
		lastCmdTextLen = strlen(lastCmdText);
	}

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
			if (strncmp(lastCmdText, curTok->childs[i]->text, lastCmdTextLen) == 0) {
				if (state == 0) {
					++alternatives;
					lastAlternativeTok = curTok->childs[i];
				} else if (state == 1) {
					cli_print_token(curTok->childs[i]);
				}
			}
		}

		// Choose what to do according to the number
		// of alternatives found on state 0
		if (state == 0) {
			if (alternatives == 0) {
				break; // Nothing to do, don't bother with state 1
			} else if (alternatives == 1) {
				return lastAlternativeTok->text;
			} else {
				printf("\n\r"); // Go to next line before printing alternatives
				continue;
			}
		}
	}

	return NULL; // No unique alternative
}

// ===================
//       EXTERN
// ===================

/**
 * @brief Init module
 * @return 0
 */
int cli_init(void)
{
	DEBUG_ENABLE(ERROR);
	//DEBUG_ENABLE(PARSER);
	//DEBUG_ENABLE(FINDER);
	//DEBUG_ENABLE(AUTOC);

	// Empty token list
	memset(tokenList, 0, sizeof(tokenList));

	// Add root children
	cli_add_token(CLI_ROOT_TOKEN_NAME, "");

	// Init LineBuffer
	lb_init();
	lb_set_valid_line_callback(&cli_execute_lb);
	lb_set_autocomplete_callback(&cli_autocomplete_lb);
	return 0;
}

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
 * @brief Give a string containing the version of CLI
 * @return String pointer
 */
const char * cli_get_version(void)
{
	return cliVersionName;
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
		DPRINTF(ERROR, "Unable to add token \"%s\", maximum reach: %d\n\r", curTok->text, CLI_MAX_TOKEN_COUNT);
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
	int argc = parent->mandatoryArgc + parent->optionalArgc;
	if (argc > 0) {
		DPRINTF(ERROR, "Unable to add children for token \"%s\", parent has %d arguments\n\r", parent->text, argc);
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

	DPRINTF(ERROR, "Unable to add children for token \"%s\", parent has no empty child (CLI_MAX_CHILDS = %d)\n\r", parent->text, CLI_MAX_CHILDS);
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
		DPRINTF(ERROR, "Unable to set callback for token \"%s\", token is not a leaf\n\r", curTok->text);
		return -1;
	}
	curTok->callback = callback;
	return 0;
}

/**
 * @brief Set the argument counts for this token
 *
 * @param curTok Pointer
 * @param mandatoryArgc int
 * @param optionalArgc int
 *
 * @return 0: ok, -1: Error
 */
int cli_set_argc(cli_token * curTok, int mandatoryArgc, int optionalArgc)
{
	if (!cli_is_token_a_leaf(curTok)) {
		DPRINTF(ERROR, "Can't set argument count for token \"%s\": token is not a leaf\n\r", curTok->text);
		return -1;
	}

	if ((mandatoryArgc < 0) || (optionalArgc < 0)) {
		DPRINTF(ERROR, "Invalid argument count for token \"%s\": mandatory=%d, optional=%d\n\r",
				curTok->text, curTok->mandatoryArgc, curTok->optionalArgc);
		return -1;
	}

	curTok->mandatoryArgc = mandatoryArgc;
	curTok->optionalArgc  = optionalArgc;
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
 * @brief Callback function for LineBuffer
 * @details The definition of this function must follow
 * the one defined in LineBuffer
 * @see lb_autocomplete_callback_t
 *
 * @param str The input command string
 * @param len The length of the str
 * @param outBuffer The buffer where we write the completion
 *
 * @return The result of the command
 */
int cli_autocomplete_lb(const char * str, int len, char * outBuffer, int outBufferMaxLen)
{
	char   cmdEdit[CLI_CMD_MAX_LEN]; // Editable copy of str
	char * cmdText[CLI_CMD_MAX_TOKEN];
	int    cmdTextCount;
	int    countToAdd, countAlreadyWrote;

	// Copy incomming buffer
	cli_strcpy_safe(cmdEdit, str, CLI_CMD_MAX_LEN);

	// PARSER (Note: cmdTextCount can be 0)
	cmdTextCount = cli_parse_cmd_text(cmdEdit, cmdText);

	// Search for alternatives
	char * pText = cli_autocomplete(cmdText, cmdTextCount);
	if (pText == NULL) {
		DPRINTF(AUTOC, "No unique alternative found\n\r");
		return 0; // Nothing added
	}

	countAlreadyWrote = strlen(cmdText[cmdTextCount - 1]); // size of what user wrote (Ex: 4 -> "conf" for "config")
	countToAdd        = strlen(pText) - countAlreadyWrote; // size we have to write (Ex: 2 -> "ig" for "conf")

	// Check overflow
	if (countToAdd > outBufferMaxLen) {
		// Not enough space to autocomplete, do nothing
		DPRINTF(ERROR, "Not enough space in line buffer for autocompletion\n\r");
		return 0;
	}

	// Append the text to add and update max len
	cli_strcpy_safe(outBuffer, pText + countAlreadyWrote, outBufferMaxLen);
	outBufferMaxLen -= countToAdd;
	DPRINTF(AUTOC, "Adding %d bytes\n\r", countToAdd);

	// Append an extra space (" ") if there is enough memory
	if (outBufferMaxLen >= 1) {
		DPRINTF(AUTOC, "Adding extra space\n\r");
		cli_strcpy_safe(outBuffer + countToAdd, " ", outBufferMaxLen);
		countToAdd += 1;
	}

	return countToAdd;
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
int cli_execute_lb(const char * str, int len)
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

	return cli_execute(cmdText, cmdTextCount);
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

/**
 * @brief Exit CLI
 *
 * @param byte The value of the caracter
 */
void cli_exit(void)
{
	lb_exit();
}