#ifndef VIEWPORT_H
#define VIEWPORT_H

#define VIEWPORT_MODE_PLANAR	1
#define VIEWPORT_MODE_SPHERICAL	2

bool viewport_init (void);
void viewport_destroy (void);
void viewport_zoom_in (const int screen_x, const int screen_y);
void viewport_zoom_out (const int screen_x, const int screen_y);
void viewport_scroll (const int dx, const int dy);
void viewport_hold_start (const int screen_x, const int screen_y);
void viewport_hold_move (const int screen_x, const int screen_y);
void viewport_center_at (const int screen_x, const int screen_y);
void viewport_tilt (const int dy);
void viewport_rotate (const int dx);
void viewport_resize (const unsigned int wd, const unsigned int ht);
void viewport_render (void);
void viewport_gl_setup_world (void);
bool viewport_within_world_bounds (void);
void viewport_get_bbox (double **bx, double **by);
void viewport_mode_set (int mode);
int viewport_mode_get (void);
bool point_inside_frustum (float x, float y);

unsigned int viewport_get_ht (void);
unsigned int viewport_get_wd (void);
double viewport_get_center_x (void);
double viewport_get_center_y (void);
int viewport_get_tile_top (void);
int viewport_get_tile_left (void);
int viewport_get_tile_right (void);
int viewport_get_tile_bottom (void);

#endif
