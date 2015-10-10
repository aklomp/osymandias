void mat_identity (float *matrix);
void mat_translate (float *matrix, float dx, float dy, float dz);
void mat_rotate (float *matrix, float x, float y, float z, float angle);
void mat_multiply (float *matrix, const float *a, const float *b);
