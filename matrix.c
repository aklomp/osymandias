#include <math.h>

#include "matrix.h"

void
mat_translate (double *matrix, double dx, double dy, double dz)
{
	matrix[0] = 1.0;
	matrix[1] = 0.0;
	matrix[2] = 0.0;
	matrix[3] = 0.0;
	matrix[4] = 0.0;
	matrix[5] = 1.0;
	matrix[6] = 0.0;
	matrix[7] = 0.0;
	matrix[8] = 0.0;
	matrix[9] = 0.0;
	matrix[10] = 1.0;
	matrix[11] = 0.0;
	matrix[12] = dx;
	matrix[13] = dy;
	matrix[14] = dz;
	matrix[15] = 1.0;
}

static void
normalize (double *x, double *y, double *z)
{
	double d = sqrt((*x) * (*x) + (*y) * (*y) + (*z) * (*z));
	*x /= d;
	*y /= d;
	*z /= d;
}

void
mat_rotate (double *matrix, double x, double y, double z, double angle)
{
	normalize(&x, &y, &z);

	double s = sin(angle);
	double c = cos(angle);
	double m = 1.0 - c;

	matrix[0] = m * x * x + c;
	matrix[1] = m * x * y - z * s;
	matrix[2] = m * z * x + y * s;
	matrix[3] = 0.0;
	matrix[4] = m * x * y + z * s;
	matrix[5] = m * y * y + c;
	matrix[6] = m * y * z - x * s;
	matrix[7] = 0.0;
	matrix[8] = m * z * x - y * s;
	matrix[9] = m * y * z + x * s;
	matrix[10] = m * z * z + c;
	matrix[11] = 0.0;
	matrix[12] = 0.0;
	matrix[13] = 0.0;
	matrix[14] = 0.0;
	matrix[15] = 1.0;
}

void
mat_scale (double *matrix, double x, double y, double z)
{
	matrix[0] = x;
	matrix[1] = 0.0;
	matrix[2] = 0.0;
	matrix[3] = 0.0;
	matrix[4] = 0.0;
	matrix[5] = y;
	matrix[6] = 0.0;
	matrix[7] = 0.0;
	matrix[8] = 0.0;
	matrix[9] = 0.0;
	matrix[10] = z;
	matrix[11] = 0.0;
	matrix[12] = 0.0;
	matrix[13] = 0.0;
	matrix[14] = 0.0;
	matrix[15] = 1.0;
}

void
mat_ortho (double *matrix, double left, double right, double bottom, double top, double near, double far)
{
	matrix[0] = 2.0 / (right - left);
	matrix[1] = 0.0;
	matrix[2] = 0.0;
	matrix[3] = 0.0;
	matrix[4] = 0.0;
	matrix[5] = 2.0 / (top - bottom);
	matrix[6] = 0.0;
	matrix[7] = 0.0;
	matrix[8] = 0.0;
	matrix[9] = 0.0;
	matrix[10] = -2.0 / (far - near);
	matrix[11] = 0.0;
	matrix[12] = -(right + left) / (right - left);
	matrix[13] = -(top + bottom) / (top - bottom);
	matrix[14] = -(far + near) / (far - near);
	matrix[15] = 1.0;
}

void
mat_frustum (double *matrix, double angle_of_view, double aspect_ratio, double z_near, double z_far)
{
	matrix[0] = 1.0 / tan(angle_of_view);
	matrix[1] = 0.0;
	matrix[2] = 0.0;
	matrix[3] = 0.0;
	matrix[4] = 0.0;
	matrix[5] = aspect_ratio / tan(angle_of_view);
	matrix[6] = 0.0;
	matrix[7] = 0.0;
	matrix[8] = 0.0;
	matrix[9] = 0.0;
	matrix[10] = -(z_far + z_near) / (z_far - z_near);
	matrix[11] = -1.0;
	matrix[12] = 0.0;
	matrix[13] = 0.0;
	matrix[14] = -2.0 * z_far * z_near / (z_far - z_near);
	matrix[15] = 0.0;
}

void
mat_vec32_multiply (float *vector, const double *m, const float *v)
{
	double result[4];
	for (int i = 0; i < 4; i++) {
		double total = 0.0;
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
mat_vec64_multiply (double *vector, const double *m, const double *v)
{
	double result[4];
	for (int i = 0; i < 4; i++) {
		double total = 0.0;
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
mat_multiply (double *matrix, const double *a, const double *b)
{
	double result[16];
	for (int c = 0; c < 4; c++) {
		for (int r = 0; r < 4; r++) {
			int index = c * 4 + r;
			double total = 0;
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

void
mat_invert (double *matrix, const double *m)
{
	double a00 = m[0],  a01 = m[1],  a02 = m[2],  a03 = m[3];
	double a10 = m[4],  a11 = m[5],  a12 = m[6],  a13 = m[7];
	double a20 = m[8],  a21 = m[9],  a22 = m[10], a23 = m[11];
	double a30 = m[12], a31 = m[13], a32 = m[14], a33 = m[15];

	double b00 = a00 * a11 - a01 * a10;
	double b01 = a00 * a12 - a02 * a10;
	double b02 = a00 * a13 - a03 * a10;
	double b03 = a01 * a12 - a02 * a11;
	double b04 = a01 * a13 - a03 * a11;
	double b05 = a02 * a13 - a03 * a12;
	double b06 = a20 * a31 - a21 * a30;
	double b07 = a20 * a32 - a22 * a30;
	double b08 = a20 * a33 - a23 * a30;
	double b09 = a21 * a32 - a22 * a31;
	double b10 = a21 * a33 - a23 * a31;
	double b11 = a22 * a33 - a23 * a32;

	double det = b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;

	matrix[0]  = a11 * b11 - a12 * b10 + a13 * b09;
	matrix[1]  = a02 * b10 - a01 * b11 - a03 * b09;
	matrix[2]  = a31 * b05 - a32 * b04 + a33 * b03;
	matrix[3]  = a22 * b04 - a21 * b05 - a23 * b03;
	matrix[4]  = a12 * b08 - a10 * b11 - a13 * b07;
	matrix[5]  = a00 * b11 - a02 * b08 + a03 * b07;
	matrix[6]  = a32 * b02 - a30 * b05 - a33 * b01;
	matrix[7]  = a20 * b05 - a22 * b02 + a23 * b01;
	matrix[8]  = a10 * b10 - a11 * b08 + a13 * b06;
	matrix[9]  = a01 * b08 - a00 * b10 - a03 * b06;
	matrix[10] = a30 * b04 - a31 * b02 + a33 * b00;
	matrix[11] = a21 * b02 - a20 * b04 - a23 * b00;
	matrix[12] = a11 * b07 - a10 * b09 - a12 * b06;
	matrix[13] = a00 * b09 - a01 * b07 + a02 * b06;
	matrix[14] = a31 * b01 - a30 * b03 - a32 * b00;
	matrix[15] = a20 * b03 - a21 * b01 + a22 * b00;

	for (int i = 0; i < 16; i++)
		matrix[i] /= det;
}

void
mat_to_float (float *dst, const double *src)
{
	for (int i = 0; i < 16; i++)
		dst[i] = src[i];
}
