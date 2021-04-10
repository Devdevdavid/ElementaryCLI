#ifndef LINE_BUFFER_H
#define LINE_BUFFER_H

// ======================
// Includes
// ======================

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// ======================
// Constants
// ======================

#define LB_LINE_BUFFER_LENGTH 32 /**< Number of character into the buffer */
#define LB_HISTORY_COUNT      10 /**< Number of line in history */

#define LB_KEY_BACKSPACE_1  0x08 // For serial
#define LB_KEY_BACKSPACE_2  0x7F // For MacOS
#define LB_KEY_TAB          0x09
#define LB_KEY_ENTER_UNIX   0x0A
#define LB_KEY_ENTER_WIN    0x0D
#define LB_KEY_ESC          0x1B
#define LB_KEY_OPEN_BRACKET 0x5B // '['
#define LB_CODE_ARROW_UP    0x41 // 'A'
#define LB_CODE_ARROW_DOWN  0x42 // 'B'
#define LB_CODE_ARROW_RIGHT 0x43 // 'C'
#define LB_CODE_ARROW_LEFT  0x44 // 'D'

// ======================
// Typedefs and structs
// ======================

typedef int (*lb_line_callback_t)(const char * str, int len);
typedef int (*lb_autocomplete_callback_t)(const char * str, int len, char * outBuffer, int outBufferMaxLen);

// ======================
// Protoypes
// ======================

int  lb_get_cursor_pos(void);
void lb_init(void);
void lb_rx(uint8_t byte);
void lb_set_valid_line_callback(lb_line_callback_t callback);
void lb_set_autocomplete_callback(lb_autocomplete_callback_t callback);
void lb_exit(void);

#endif /* LINE_BUFFER_H */