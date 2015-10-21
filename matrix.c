#include <math.h>

void
mat_identity (float *matrix)
{
	matrix[0] = 1;
	matrix[1] = 0;
	matrix[2] = 0;
	matrix[3] = 0;
	matrix[4] = 0;
	matrix[5] = 1;
	matrix[6] = 0;
	matrix[7] = 0;
	matrix[8] = 0;
	matrix[9] = 0;
	matrix[10] = 1;
	matrix[11] = 0;
	matrix[12] = 0;
	matrix[13] = 0;
	matrix[14] = 0;
	matrix[15] = 1;
}

void
mat_translate (float *matrix, float dx, float dy, float dz)
{
	matrix[0] = 1;
	matrix[1] = 0;
	matrix[2] = 0;
	matrix[3] = 0;
	matrix[4] = 0;
	matrix[5] = 1;
	matrix[6] = 0;
	matrix[7] = 0;
	matrix[8] = 0;
	matrix[9] = 0;
	matrix[10] = 1;
	matrix[11] = 0;
	matrix[12] = dx;
	matrix[13] = dy;
	matrix[14] = dz;
	matrix[15] = 1;
}

static void
normalize (float *x, float *y, float *z)
{
	float d = sqrtf((*x) * (*x) + (*y) * (*y) + (*z) * (*z));
	*x /= d;
	*y /= d;
	*z /= d;
}

void
mat_rotate (float *matrix, float x, float y, float z, float angle)
{
	normalize(&x, &y, &z);

	float s = sinf(angle);
	float c = cosf(angle);
	float m = 1 - c;

	matrix[0] = m * x * x + c;
	matrix[1] = m * x * y - z * s;
	matrix[2] = m * z * x + y * s;
	matrix[3] = 0;
	matrix[4] = m * x * y + z * s;
	matrix[5] = m * y * y + c;
	matrix[6] = m * y * z - x * s;
	matrix[7] = 0;
	matrix[8] = m * z * x - y * s;
	matrix[9] = m * y * z + x * s;
	matrix[10] = m * z * z + c;
	matrix[11] = 0;
	matrix[12] = 0;
	matrix[13] = 0;
	matrix[14] = 0;
	matrix[15] = 1;
}

void
mat_scale (float *matrix, float x, float y, float z)
{
	matrix[0] = x;
	matrix[1] = 0;
	matrix[2] = 0;
	matrix[3] = 0;
	matrix[4] = 0;
	matrix[5] = y;
	matrix[6] = 0;
	matrix[7] = 0;
	matrix[8] = 0;
	matrix[9] = 0;
	matrix[10] = z;
	matrix[11] = 0;
	matrix[12] = 0;
	matrix[13] = 0;
	matrix[14] = 0;
	matrix[15] = 1;
}

void
mat_ortho (float *matrix, float left, float right, float bottom, float top, float near, float far)
{
	matrix[0] = 2 / (right - left);
	matrix[1] = 0;
	matrix[2] = 0;
	matrix[3] = 0;
	matrix[4] = 0;
	matrix[5] = 2 / (top - bottom);
	matrix[6] = 0;
	matrix[7] = 0;
	matrix[8] = 0;
	matrix[9] = 0;
	matrix[10] = -2 / (far - near);
	matrix[11] = 0;
	matrix[12] = -(right + left) / (right - left);
	matrix[13] = -(top + bottom) / (top - bottom);
	matrix[14] = -(far + near) / (far - near);
	matrix[15] = 1;
}

void
mat_frustum (float *matrix, float angle_of_view, float aspect_ratio, float z_near, float z_far)
{
	matrix[0] = 1.0f / tanf(angle_of_view);
	matrix[1] = 0.0f;
	matrix[2] = 0.0f;
	matrix[3] = 0.0f;
	matrix[4] = 0.0f;
	matrix[5] = aspect_ratio / tanf(angle_of_view);
	matrix[6] = 0.0f;
	matrix[7] = 0.0f;
	matrix[8] = 0.0f;
	matrix[9] = 0.0f;
	matrix[10] = (z_far + z_near) / (z_far - z_near);
	matrix[11] = 1.0f;
	matrix[12] = 0.0f;
	matrix[13] = 0.0f;
	matrix[14] = -2.0f * z_far * z_near / (z_far - z_near);
	matrix[15] = 0.0f;
}

void
mat_vec_multiply (float *vector, const float *m, const float *v)
{
	float result[4];
	for (int i = 0; i < 4; i++) {
		float total = 0.0f;
		for (int j = 0; j < 4; j++) {
			int p = j * 4 + i;
			int q = j;
			total += m[p] * v[q];
		}
		result[i] = total;
	}
	for (int i = 0; i < 4; i++)
		vector[i] = result[i];
}

void
mat_multiply (float *matrix, const float *a, const float *b)
{
	float result[16];
	for (int c = 0; c < 4; c++) {
		for (int r = 0; r < 4; r++) {
			int index = c * 4 + r;
			float total = 0;
			for (int i = 0; i < 4; i++) {
				int p = i * 4 + r;
				int q = c * 4 + i;
				total += a[p] * b[q];
			}
			result[index] = total;
		}
	}
	for (int i = 0; i < 16; i++)
		matrix[i] = result[i];
}
