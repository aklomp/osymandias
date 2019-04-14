#pragma once

#include <stdbool.h>
#include <stdint.h>

extern bool zoom_on_tick (const int64_t now);
extern void zoom_out     (const int64_t now);
extern void zoom_in      (const int64_t now);
