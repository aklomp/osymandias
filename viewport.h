#pragma once

#include <stdbool.h>
#include <stdint.h>

struct viewport {
	uint32_t height;
	uint32_t width;
};

struct screen_pos {
	uint32_t x;
	uint32_t y;
};

extern void viewport_destroy (void);
extern void viewport_zoom_in (const struct screen_pos *);
extern void viewport_zoom_out (const struct screen_pos *);
extern void viewport_scroll (const int dx, const int dy);
extern void viewport_hold_start (const struct screen_pos *);
extern void viewport_hold_move (const struct screen_pos *);
extern void viewport_center_at (const struct screen_pos *);
extern void viewport_resize (const uint32_t wd, const uint32_t ht);
extern void viewport_gl_setup_world (void);
extern void viewport_paint (void);

extern const struct viewport *viewport_get (void);
extern bool viewport_init (const uint32_t width, const uint32_t height);
