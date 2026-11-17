// Just enough of Arduino.h to build firmware/watch_v1/oled_render.h on a computer (parity test).
#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define PROGMEM
#define pgm_read_byte(p) (*(const uint8_t *)(p))
#define pgm_read_word(p) (*(const uint16_t *)(p))
#define pgm_read_pointer(p) (*(p))
