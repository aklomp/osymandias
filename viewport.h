#pragma once

#include <stdbool.h>

struct screen_pos {
	unsigned int x;
	unsigned int y;
};

bool viewport_init (void);
void viewport_destroy (void);
void viewport_zoom_in (const struct screen_pos *);
void viewport_zoom_out (const struct screen_pos *);
void viewport_scroll (const int dx, const int dy);
void viewport_hold_start (const struct screen_pos *);
void viewport_hold_move (const struct screen_pos *);
void viewport_center_at (const struct screen_pos *);
void viewport_resize (const unsigned int wd, const unsigned int ht);
void viewport_gl_setup_world (void);
void viewport_paint (void);

unsigned int viewport_get_ht (void);
unsigned int viewport_get_wd (void);
