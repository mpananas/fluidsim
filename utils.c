#include "utils.h"
#include <stdlib.h>
#include <math.h>

#define PI 3.14159265359

float frand(float min, float max)
{
	return ((float)rand() / RAND_MAX) * (max - min) + min;
}

void frand2d(float* x, float* y)
{
	float t = frand(-PI, PI);
	*x = sin(t);
	*y = cos(t);
}

inline float dot(float x1, float y1, float x2, float y2)
{
	return (x1 * x2 + y1 * y2);
}

float fclamp(float t, float min, float max)
{
	const float s = t < min ? min : t;
	return (s > max ? max : s);
}

int iclamp(int t, int min, int max)
{
	const int s = t < min ? min : t;
	return s > max ? max : s;
}

int fsgn(float t)
{
	return (t > 0) - (t < 0);
}

int isgn(int t)
{
	return (t > 0) - (t < 0);
}
