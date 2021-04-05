#ifndef DEBUG_H
#define DEBUG_H

#ifdef CLI_C
// Variable declaration
int debugCli = 0;
#define DEBUG_VAR_NAME debugCli

// Flag declaration
#define DEBUG_INFO   0x0001
#define DEBUG_ERROR  0x0002
#define DEBUG_FINDER 0x0004
#define DEBUG_PARSER 0x0008

#elif defined(LINE_BUFFER_C)
// Variable declaration
int debugLineBuffer = 0;
#define DEBUG_VAR_NAME debugLineBuffer

// Flag declaration
#define DEBUG_INFO     0x0001
#define DEBUG_ERROR    0x0002

#else
#error "No context found for debug.h"
#endif

// Common to all contexts
#ifdef DEBUG

#define DEBUG_OUTPUT        printf                             /**< Define where output text goes */
#define DEBUG_ENABLE(flag)  DEBUG_VAR_NAME |= DEBUG_##flag     /**< Enable a flag */
#define DEBUG_DISABLE(flag) DEBUG_VAR_NAME &= ~DEBUG_##flag    /**< Disable a flag */
#define DEBUG_BLOC(flag)    if (DEBUG_VAR_NAME & DEBUG_##flag) /**< Test if flag is enabled */
#define DPRINTF(flag, ...)                                     /**< Print string if flag is enabled */ \
	DEBUG_BLOC(flag)                                                                                   \
	{                                                                                                  \
		DEBUG_OUTPUT("[" #flag "] " __VA_ARGS__);                                                      \
	}

#else

// Do nothing if not in DEBUG
#define DEBUG_OUTPUT
#define DEBUG_ENABLE(flag)
#define DEBUG_DISABLE(flag)
#define DEBUG(flag)
#define DPRINTF(flag, ...)

#endif /* DEBUG */

#endif /* DEBUG_H */