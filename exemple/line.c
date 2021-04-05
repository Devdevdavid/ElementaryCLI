#include <ncurses.h>
#include <signal.h>
#include <unistd.h>

#include "lineBuffer.h"

static volatile int keepRunning = 1;

void sigint_handler(int dummy)
{
	keepRunning = 0;
	printf("\n\r- Quitting...\n\r");
}

int main(void)
{
	uint8_t byte;

	initscr();
	endwin();
	noecho();
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	timeout(100);

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