#pragma once

extern void mat_translate (float *matrix, float dx, float dy, float dz);
extern void mat_rotate (float *matrix, float x, float y, float z, float angle);
extern void mat_scale (float *matrix, float x, float y, float z);
extern void mat_ortho (float *matrix, float left, float right, float bottom, float top, float near, float far);
extern void mat_frustum (float *matrix, float angle_of_view, float aspect_ratio, float z_near, float z_far);
extern void mat_vec_multiply (float *vector, const float *m, const float *v);
extern void mat_multiply (float *matrix, const float *a, const float *b);
extern void mat_invert (float *matrix, const float *m);
