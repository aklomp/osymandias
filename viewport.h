#ifndef VIEWPORT_H
#define VIEWPORT_H

void viewport_init (void);
void viewport_destroy (void);
void viewport_zoom_in (const int screen_x, const int screen_y);
void viewport_zoom_out (const int screen_x, const int screen_y);
void viewport_scroll (const int dx, const int dy);
void viewport_center_at (const int screen_x, const int screen_y);
void viewport_reshape (const unsigned int width, const unsigned int height);
void viewport_render (void);

#endif
