// Based on: https://github.com/espressif/esp-idf/blob/fe904085fbae405c6b5968493371da50017ece03/components/log/log_buffers.c

#include <ctype.h>
#include <stdio.h>

void hexdump(const char *tag, const void *buffer, uint16_t buff_len);