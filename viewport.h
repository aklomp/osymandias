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
void viewport_tilt (const int dy);
void viewport_rotate (const int dx);
void viewport_reshape (const unsigned int width, const unsigned int height);
void viewport_render (void);
void viewport_gl_setup_screen (void);
void viewport_gl_setup_world (void);
bool viewport_within_world_bounds (void);
void viewport_screen_to_world (double sx, double sy, double *wx, double *wy);
void viewport_world_to_screen (double wx, double wy, double *sx, double *sy);
void viewport_get_frustum (double **wx, double **wy);
void viewport_get_bbox (double **bx, double **by);
bool point_inside_frustum (double x, double y);

unsigned int viewport_get_ht (void);
unsigned int viewport_get_wd (void);
double viewport_get_center_x (void);
double viewport_get_center_y (void);
float viewport_get_rot (void);
float viewport_get_tilt (void);
int viewport_get_tile_top (void);
int viewport_get_tile_left (void);
int viewport_get_tile_right (void);
int viewport_get_tile_bottom (void);

#endif
