#include <ncurses.h>
#include <signal.h>
#include <unistd.h>

#include "cli.h"
#include "lineBuffer.h"

// Tell if main loop should keep running
static volatile int keepRunning = 1;

/**
 * @brief Handler to stop app on ctrl-c
 *
 * @param dummy unused
 */
void sigint_handler(int dummy)
{
	keepRunning = 0;
	printf("\n\r- Quitting...\n\r");
}

// ================
// CMD CALLBACKS
// ================

int cli_cb_exit(int argc, char * argv[])
{
	sigint_handler(SIGINT);
	return 0;
}

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
static int create_cli_commands(void)
{
	cli_token * tokRoot = cli_get_root_token();
	cli_token * tokLan;
	cli_token * tokIpSet;
	cli_token * curTok;

	// Create commands
	curTok = cli_add_token("exit", "Exit application");
	cli_set_callback(curTok, &cli_cb_exit);
	cli_add_children(tokRoot, curTok);

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

int main(void)
{
	uint8_t byte;

	// Init signal handler
	signal(SIGINT, sigint_handler);

	// Init ncurses stuff
	initscr();
	endwin();
	noecho();
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	timeout(100);

	// Using getch now allows us to see firsts printf()
	getch();

	// motd
	printf("Exemple using %s\n\r", cli_get_version());

	cli_init();
	create_cli_commands();

	while (keepRunning) {
		byte = getch();
		if (byte == 0xFF) {
			continue;
		}
		cli_rx(byte);
	}

	return 0;
}