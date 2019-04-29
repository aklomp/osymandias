#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "viewport.h"

extern bool pan_on_tick        (const int64_t now);
extern void pan_on_button_down (const struct viewport_pos *pos, const int64_t now);
extern bool pan_on_button_move (const struct viewport_pos *pos, const int64_t now);
extern void pan_on_button_up   (const struct viewport_pos *pos, const int64_t now);
