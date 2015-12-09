#ifndef VIEWPORT_H
#define VIEWPORT_H

bool viewport_init (void);
void viewport_destroy (void);
void viewport_zoom_in (const int screen_x, const int screen_y);
void viewport_zoom_out (const int screen_x, const int screen_y);
void viewport_scroll (const int dx, const int dy);
void viewport_hold_start (const int screen_x, const int screen_y);
void viewport_hold_move (const int screen_x, const int screen_y);
void viewport_center_at (const int screen_x, const int screen_y);
void viewport_resize (const unsigned int wd, const unsigned int ht);
void viewport_render (void);
void viewport_gl_setup_world (void);

unsigned int viewport_get_ht (void);
unsigned int viewport_get_wd (void);

#endif
