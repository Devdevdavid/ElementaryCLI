#include <ncurses.h>
#include <signal.h>
#include <unistd.h>

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

int main(void)
{
	uint8_t byte;

	// Init ncurses stuff
	initscr();
	endwin();
	noecho();
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	timeout(100);

	// Using getch now allows us to see firsts printf()
	getch();

	printf(" - Starting...\n\r");
	signal(SIGINT, sigint_handler);

	lb_init();

	while (keepRunning) {
		byte = getch();
		if (byte == 0xFF) {
			continue;
		}
		if (lb_rx(byte) == 1) {
		}
	}

	return 0;
}