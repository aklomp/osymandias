#pragma once

extern void mat_translate (double *matrix, double dx, double dy, double dz);
extern void mat_rotate (double *matrix, double x, double y, double z, double angle);
extern void mat_scale (double *matrix, double x, double y, double z);
extern void mat_ortho (double *matrix, double left, double right, double bottom, double top, double near, double far);
extern void mat_frustum (double *matrix, double angle_of_view, double aspect_ratio, double z_near, double z_far);
extern void mat_vec32_multiply (float *vector, const double *m, const float *v);
extern void mat_vec64_multiply (double *vector, const double *m, const double *v);
extern void mat_multiply (double *matrix, const double *a, const double *b);
extern void mat_invert (double *matrix, const double *m);
extern void mat_to_float (float *dst, const double *src);
