#pragma once

#include <stdbool.h>

#include <vec/vec.h>

bool camera_init (void);
void camera_destroy (void);
void camera_tilt (const float radians);
void camera_rotate (const float radians);
void camera_projection (const int screen_wd, const int screen_ht);
void camera_setup (void);
bool camera_is_tilted (void);
bool camera_is_rotated (void);
bool camera_visible_quad (const union vec **coords);
float camera_distance_squared (const union vec *);
union vec camera_distance_squared_quad (const union vec x, const union vec y, const union vec z);
union vec camera_distance_squared_quadedge (const union vec x, const union vec y, const union vec z);
const float *camera_mat_viewproj (void);
const float *camera_mat_viewproj_inv (void);
const float *camera_pos (void);
void camera_unproject (union vec *, union vec *, const unsigned int x, const unsigned int y, const unsigned int screen_wd, const unsigned int screen_ht);
