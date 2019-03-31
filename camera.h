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
const float *camera_mat_viewproj (void);
const float *camera_mat_viewproj_inv (void);
const float *camera_pos (void);
void camera_unproject (union vec *, union vec *, const unsigned int x, const unsigned int y, const unsigned int screen_wd, const unsigned int screen_ht);
