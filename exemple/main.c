#include "cli.h"

/**
 * @brief Default callback for leaf tokens
 *
 * @param argc Argument count
 * @param argv Argument values
 *
 * @return The status of the function
 */
int print_args(int argc, char * argv[])
{
	printf("print_args() Found %d args:\n\r", argc);
	for (int i = 0; i < argc; ++i) {
		printf("\t%s\n", argv[i]);
	}
	return 0;
}

/**
 * @brief Create the command line interface
 */
static int create_cli(void)
{
	cli_token * tokRoot = cli_get_root_token();
	cli_token * tokLan;
	cli_token * tokIpSet;
	cli_token * curTok;

	// Create commands
	cli_init();
	tokLan = cli_add_token("lan", "LAN configuration");
	{
		curTok = cli_add_token("show", "Show configuration");
		cli_set_callback(curTok, &print_args);
		cli_add_children(tokLan, curTok);

		tokIpSet = cli_add_token("set", "Define new configuration");
		{
			curTok = cli_add_token("ip", "<address> Set IP adress");
			cli_set_callback(curTok, &print_args);
			cli_set_argc(curTok, 1);
			cli_add_children(tokIpSet, curTok);

			curTok = cli_add_token("gateway", "<address> Set gateway adress");
			cli_set_callback(curTok, &print_args);
			cli_set_argc(curTok, 1);
			cli_add_children(tokIpSet, curTok);

			curTok = cli_add_token("mask", "<address> Set network mask");
			cli_set_callback(curTok, &print_args);
			cli_set_argc(curTok, 1);
			cli_add_children(tokIpSet, curTok);

			curTok =
			cli_add_token("proxy", "<address> <port> Set the proxy location");
			cli_set_callback(curTok, &print_args);
			cli_set_argc(curTok, 2);
			cli_add_children(tokIpSet, curTok);
		}
		cli_add_children(tokLan, tokIpSet);
	}
	cli_add_children(tokRoot, tokLan);
	return 0;
}

/**
 * @brief Exemple for ElementaryCLI
 *
 * @return 0
 */
int main(int argc, char const * argv[])
{

	char   cmdEdit[CLI_CMD_MAX_LEN];
	char * cmdText[CLI_CMD_MAX_TOKEN];
	int    cmdTextCount;

	printf("Exemple using %s\n\r", cli_get_version());

	create_cli();

	// Copy incomming buffer
	cli_strcpy_safe(cmdEdit, argv[1], CLI_CMD_MAX_LEN);

	// PARSER
	cmdTextCount = cli_parse_cmd_text(cmdEdit, cmdText);
	if (cmdTextCount <= 0) {
		printf("Parse error!\n\r");
		return -1;
	}

	// Search for alternatives
	char * pText = cli_auto_complete(cmdText, cmdTextCount);
	if (pText != NULL) {
		printf("Alternative: %s\n\r", pText);
	}

	return cli_execute_command(cmdText, cmdTextCount);
}