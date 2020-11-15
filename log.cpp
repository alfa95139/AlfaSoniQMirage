
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "Arduino.h"

#include "log.h"

#define NAME_STR_LEN 11
#define MAX_LINE_MESSAGE_LENGTH 64

bool _emergency_triggered = false;
char debug_str[NAME_STR_LEN];
bool debug_enable = true;

char tmp_log_text[256];
char tmp_message[MAX_LINE_MESSAGE_LENGTH + 1];

void set_log(const char* s) {
  strlcpy(debug_str, s, NAME_STR_LEN);
  for (int i = strlen(debug_str); i < NAME_STR_LEN - 1; i++)
    debug_str[i] = ' ';
  debug_str[NAME_STR_LEN - 1] = 0;
}

void set_debug_enable(bool enable) {
  debug_enable = enable;
}

bool was_emergency_triggered() {
  bool tmp = _emergency_triggered;
  _emergency_triggered = false;
  return tmp;
}

void _print_header(const char* type) {
  Serial.printf("[%08.3f] [%s | %s] : ", millis() / 1000.0, debug_str, type);
}

void _base_print(const char* type, const char* fmt, va_list args) {
  vsnprintf(tmp_log_text, 255, fmt, args);
  tmp_log_text[255] = 0;
  int length = strlen(tmp_log_text);

  int i = 0;
  int bytes_printed = 0;
  char* current_buffer = tmp_log_text;
  while (bytes_printed < length) {
    char c = current_buffer[i];
    if (c == '\n' || c == '\r') {

      // Print if we have data
      if (i > 0) {
        _print_header(type);
        Serial.write(current_buffer, i);
        Serial.write('\n');

        // increment counters
        bytes_printed += i;
        current_buffer += i;
      }

      // set to one to skip the byte we just read
      i = 1;

      // Tell the loop that we just finished another byte (the newline or carriage return)
      bytes_printed++;

    } else if (i >= MAX_LINE_MESSAGE_LENGTH) {

      if (i > 0) {
        _print_header(type);
        Serial.write(current_buffer, i);
        Serial.write('\n');

        // increment counters
        bytes_printed += i;
        current_buffer += i;
      }
      i = 0;
    } else if (c == 0) {

      if (i > 0) {
        _print_header(type);
        Serial.write(current_buffer, i);
        Serial.write('\n');
      }

      // We're done here - received null terminator
      break;
    } else {
      i++;
    }
  }
}

void log_debug(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  if (debug_enable)
    _base_print("DEBUG", fmt, args);
  va_end(args);
}

void log_info(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  _base_print(" INFO", fmt, args);
  va_end(args);
}

void log_warning(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  _base_print(" WARN", fmt, args);
  va_end(args);
}

void log_error(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  _base_print("ERROR", fmt, args);
  va_end(args);
}

void log_emergency(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  _base_print("EMERGENCY", fmt, args);
  va_end(args);
  _emergency_triggered = true;
}
