#include "lineBuffer.h"

#define LINE_BUFFER_C
#include "debug.h"

// Global variables
typedef struct {
	char lineBufferTable[LB_HISTORY_COUNT][LB_LINE_BUFFER_LENGTH]; /**< Buffer to store the state of the line */

	int                historyIndex;   /**< The current lineBuffer index under edition, history will be saved here after processing */
	int                explorerIndex;  /**< Index controlled by user when explorating history */
	char *             curLineBuffer;  /**< The line currently under edition by user */
	uint8_t            lineSize;       /**< Size of the line (without ending '\0') */
	char *             pCurPos;        /**< Current position of the cursor */
	uint8_t            isEscaping : 1; /**< Tell if next bytes will be managed as escaped command */
	uint8_t            escPos;         /**< Current position in the escBuffer */
	lb_line_callback_t lineCallback;   /**< Function called when user valid a line */
} lb_handle_t;
lb_handle_t lbHandle;

// ===================
//      TOOLS
// ===================

/**
 * @brief Increment (+1) or decrement (-1) index value and
 * keep output value inside [0; maxRange[
 *
 * @param index incomming value
 * @param add The value to decrement/increment (typically +1 or -1)
 * @param maxRange The maximum value
 * @return The updated index
 */
static int lb_loop_index_operation(int index, int add, int maxRange)
{
	index += add;
	if (index < 0) {
		return index + maxRange;
	} else if (index >= maxRange) {
		return index - maxRange;
	} else {
		return index;
	}
}

// ===================
//      STATIC
// ===================

/**
 * @brief Insert the character toInsert into .curLineBuffer
 * @note Do nothing if overflow is detected
 * @param toInsert Character to insert
 */
static void lb_insert_at_cursor(char toInsert)
{
	char * pBuffer = lbHandle.pCurPos;
	char * pEnd;
	char   backup;

	// Check size before inserting
	if ((lbHandle.lineSize + 1) >= LB_LINE_BUFFER_LENGTH) {
		DPRINTF(ERROR, "Line buffer is full ! (LB_LINE_BUFFER_LENGTH = %d)", LB_LINE_BUFFER_LENGTH);
		return;
	}

	// Define the new ending line
	++lbHandle.lineSize;
	pEnd = &lbHandle.curLineBuffer[lbHandle.lineSize];

	// Slide characters
	while (pBuffer <= pEnd) {
		backup   = *pBuffer;
		*pBuffer = toInsert;
		toInsert = backup;
		pBuffer++;
	}

	// Slide cursor position
	++lbHandle.pCurPos;
}

/**
 * @brief Remove the character at .pCurPos
 * @note Do nothing if line is empty
 */
static void lb_remove_at_cursor(void)
{
	char * pBuffer;
	char * pEnd;

	// Check size before removing
	if (((int) lbHandle.lineSize - 1) < 0) {
		return;
	}

	// Can't remove char if positionned at first char
	if (lbHandle.pCurPos <= lbHandle.curLineBuffer) {
		return;
	}

	--lbHandle.pCurPos;
	pBuffer = lbHandle.pCurPos;
	pEnd    = lbHandle.curLineBuffer + lbHandle.lineSize;
	--lbHandle.lineSize;

	// Slide characters
	while (pBuffer <= pEnd) {
		*pBuffer = *(pBuffer + 1);
		pBuffer++;
	}
}

/**
 * @brief Copy an historic line to the current line buffer
 *
 * @param index Index of the history line buffer to copy
 * @return 0: ok, -1: Nothing on history
 */
static int lb_use_history(void)
{
	// Shortcuts
	uint8_t index       = lbHandle.explorerIndex;
	char *  historyLine = lbHandle.lineBufferTable[index];

	// Is the pointed history line empty ?
	if (historyLine[0] == '\0') {
		return -1;
	}

	// Copy content
	if (lbHandle.curLineBuffer == historyLine) {
		// We came back to curLine, empty the line buffer
		lbHandle.curLineBuffer[0] = '\0';
	} else {
		// Copy history to curLine
		strncpy(lbHandle.curLineBuffer, historyLine, LB_LINE_BUFFER_LENGTH);
	}

	// Update positions
	lbHandle.lineSize = strlen(lbHandle.curLineBuffer);
	lbHandle.pCurPos  = lbHandle.curLineBuffer + lbHandle.lineSize;
	return 0;
}

/**
 * @brief Execute the actions assciated to escaped codes
 *
 * @param byte The escaped code
 * @return 0: OK, -1: unsupported code
 */
static int lb_exec_escaped_code(uint8_t byte)
{
	int tmp;

	switch (byte) {
	case LB_CODE_ARROW_UP:
	case LB_CODE_ARROW_DOWN:
		// Decide if we go up (-1) or down (+1) in history
		tmp                    = (byte == LB_CODE_ARROW_UP) ? -1 : +1;
		lbHandle.explorerIndex = lb_loop_index_operation(lbHandle.explorerIndex, tmp, LB_HISTORY_COUNT);
		if (lb_use_history() == -1) {
			// If nothing to see there (empty strings), come back to previous value as if nothing happened
			lbHandle.explorerIndex = lb_loop_index_operation(lbHandle.explorerIndex, -tmp, LB_HISTORY_COUNT);
		}
		break;
	case LB_CODE_ARROW_RIGHT:
		// Increment cursor
		tmp = lbHandle.pCurPos - lbHandle.curLineBuffer;
		if (tmp <= (lbHandle.lineSize - 1)) {
			++lbHandle.pCurPos;
		}
		break;
	case LB_CODE_ARROW_LEFT:
		// Decrement cursor
		if (lbHandle.pCurPos > lbHandle.curLineBuffer) {
			--lbHandle.pCurPos;
		}
		break;
	default:
		DPRINTF(ERROR, "Unsupported escape code : 0x%02X\n\r", byte);
		return -1;
	}
	return 0;
}

/**
 * @brief Handle the escaped codes with a simple state machine
 *
 * @param byte The incomming new character
 * @return [description]
 */
static void lb_handle_escaped(uint8_t byte)
{
	++lbHandle.escPos;

	// Check '['
	if (lbHandle.escPos == 1) {
		// If 2nd character is ok, wait for the next one
		if (byte == LB_KEY_OPEN_BRACKET) {
			return;
		}
	} else if (lbHandle.escPos == 2) {
		lb_exec_escaped_code(byte);
	}

	// Reset escape handler
	lbHandle.isEscaping = false;
	lbHandle.escPos     = 0;
}

/**
 * @brief Compare the current line buffer with the previous in the history
 * @return 0: previous and current are identical
 */
static int lb_cmp_curline_prevline()
{
	char * prevLine;
	char * curLine;
	int    index;

	index = lb_loop_index_operation(lbHandle.historyIndex, -1, LB_HISTORY_COUNT);

	prevLine = lbHandle.lineBufferTable[index];
	curLine  = lbHandle.curLineBuffer;

	return strncmp(prevLine, curLine, LB_LINE_BUFFER_LENGTH);
}

/**
 * @brief Save current line and go to the next one (the older)
 * @return 0
 */
static int lb_save_to_history(void)
{
	// Do not save empty lines and duplicates
	if ((lbHandle.lineSize > 0) && (lb_cmp_curline_prevline() != 0)) {
		// Go to next lineBuffer
		lbHandle.historyIndex  = lb_loop_index_operation(lbHandle.historyIndex, +1, LB_HISTORY_COUNT);
		lbHandle.curLineBuffer = lbHandle.lineBufferTable[lbHandle.historyIndex];
	}

	// Reset positions
	lbHandle.pCurPos = lbHandle.curLineBuffer;
	memset(lbHandle.curLineBuffer, 0, LB_LINE_BUFFER_LENGTH);
	lbHandle.explorerIndex = lbHandle.historyIndex;
	return 0;
}

/**
 * @brief Refresh the line displayed on the terminal
 */
static void lb_term_update(void)
{
	// [%dD set cursor pos
	printf("\x1B[1000D" // Set cursor to begin line
		   "\x1B[K"     // Kill line
		   "> %s"       // Print prompt and line
		   "\x1B[1000D" // Set cursor to begin line
		   "\x1B[%dC",  // Set cursor to actual position
		   lbHandle.curLineBuffer, 2 + lb_get_cursor_pos());
}

// ===================
//      EXTERN
// ===================

/**
 * @brief Get the current cursor position
 * @return position
 */
int lb_get_cursor_pos(void)
{
	int curPos = lbHandle.pCurPos - lbHandle.curLineBuffer;
	return (int) curPos;
}

/**
 * @brief Initialize the module
 */
void lb_init(void)
{
	DEBUG_ENABLE(ERROR);
	DEBUG_ENABLE(INFO);

	// Init handle
	memset(&lbHandle, 0, sizeof(lbHandle));
	lbHandle.curLineBuffer = lbHandle.lineBufferTable[0];
	lbHandle.pCurPos       = lbHandle.curLineBuffer;

	// Display prompt on init
	lb_term_update();
}

/**
 * @brief Process the lineBuffer once validated by user
 */
void lb_process_line(void)
{
	// Keep the actual display on line n
	// and move to n+1 to display command's results
	printf("\n\r");

	// Execute the command
	if (lbHandle.lineCallback != NULL) {
		lbHandle.lineCallback(lbHandle.curLineBuffer, lbHandle.lineSize);
	}

	// Save the command into history
	lb_save_to_history();
}

/**
 * @brief Receive incomming byte from user
 *
 * @param byte Incomming byte
 */
void lb_rx(uint8_t byte)
{
	// DPRINTF(INFO, "rx: %c (0x%02X)\n\r", byte, byte);
	if (lbHandle.isEscaping) {
		lb_handle_escaped(byte);
	} else if (byte == LB_KEY_ESC) {
		lbHandle.isEscaping = true;
	} else if (byte == LB_KEY_ENTER_WIN) {
		// Ignore this
	} else if (byte == LB_KEY_ENTER_UNIX) {
		lb_process_line();
	} else if ((byte == LB_KEY_BACKSPACE_1) || (byte == LB_KEY_BACKSPACE_2)) {
		lb_remove_at_cursor();
	} else {
		lb_insert_at_cursor((char) byte);
	}
	lb_term_update();
}

/**
 * @brief Define the function callback to call when user hit enter
 *
 * @param callback Pointer on function, can be NULL to remove callback
 */
void lb_set_valid_line_callback(lb_line_callback_t callback)
{
	lbHandle.lineCallback = callback;
}
