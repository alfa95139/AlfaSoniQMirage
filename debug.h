
#ifndef DEBUG_H
#define DEBUG_H

#include <stdarg.h>

// Set the name of the part of the program currently being logged
void set_log(const char* s);

// enable/disable debug print statements
void set_debug_enable(bool enable);

// detect if anyone called the emergency() function
bool was_emergency_triggered();

/*
 * PRINT STATEMENTS
 */

// Use this to be verbose
void log_debug(const char* fmt, ...);

// Use this to provide information
void log_info(const char* fmt, ...);

// Use this if you think something may be wrong
void log_warning(const char* fmt, ...);

// Use this if something went wrong but it's recoverable without any restart
void log_error(const char* fmt, ...);

// Use this if there is an unrecoverable error and the system must restart (like Windows BSOD)
void log_emergency(const char* fmt, ...);

#endif
