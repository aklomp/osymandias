#pragma once

void mat_identity (float *matrix);
void mat_translate (float *matrix, float dx, float dy, float dz);
void mat_rotate (float *matrix, float x, float y, float z, float angle);
void mat_scale (float *matrix, float x, float y, float z);
void mat_ortho (float *matrix, float left, float right, float bottom, float top, float near, float far);
void mat_frustum (float *matrix, float angle_of_view, float aspect_ratio, float z_near, float z_far);
void mat_vec_multiply (float *vector, const float *m, const float *v);
void mat_multiply (float *matrix, const float *a, const float *b);
void mat_invert (float *matrix, const float *m);
