#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include "simp_quadtree.h"
#include "utils.h"

#define PI 3.14159265359

static float sample_density(uint32_t index, simp_quadtree* qtree, float* pos, float h);
static void fluid_accel(uint32_t i, simp_quadtree* qtree, float* pos, float* vel, float* dens, float* col,
		float h, float rest_density, float stiffness_constant, float surface_coefficient,
		float viscosity_coefficient, float* ax, float* ay);
static float density_kernel(float dst, float h);
static float density_kernel_derivative(float dst, float h);
static float viscosity_kernel_laplacian(float dst, float h);
static float surface_tension_derivative(float dst, float h);
static float surface_tension_laplacian(float dd, float h);

static float sample_density(uint32_t index, simp_quadtree* qtree, float* pos, float h)
{
	static const float area_ratio = PI / 4.0;
	float density = density_kernel(0.0f, h);
	float x = pos[2 * index + 0];
	float y = pos[2 * index + 1];
	simp_list* list = simp_quadtree_query(qtree, x - h, y - h, x + h, y + h);
	simp_list_iter* iter = simp_list_iter_create(list);
	int j;
	while(simp_list_iter_next(iter, &j))
	{
		if(j == index) { continue; }
		float other_x = pos[2 * j + 0];
		float other_y = pos[2 * j + 1];
		float dx = other_x - x;
		float dy = other_y - y;
		float dd = dx * dx + dy * dy;
		//Check if the other point is contained inside the ball with radius h
		if(dd <= h * h)
			density += density_kernel(sqrtf(dd), h);
	}
	simp_list_iter_destroy(iter);
	simp_list_destroy(list);

	float boundary_weight = 1.0f;
	if(x - h < 0.0f || x + h > 1.0f || y - h < 0.0f || y + h > 1.0f)
	{
		float area_h = 4 * h * h;
		float min_x = fmax(0.0f, x - h);
		float min_y = fmax(0.0f, y - h);
		float max_x = fmin(1.0f, x + h);
		float max_y = fmin(1.0f, y + h);
		boundary_weight = area_h / fabs(area_ratio * (max_x - min_x) * (max_y - min_y));
	}
	return density * boundary_weight;
}

static void fluid_accel(uint32_t i, simp_quadtree* qtree, float* pos, float* vel, float* dens, float* col,
		float h, float rest_density, float stiffness_constant, float surface_coefficient,
		float viscosity_coefficient, float* ax, float* ay)
{
	col[3 * i + 0] = 1.0f;
	col[3 * i + 1] = 1.0f;
	col[3 * i + 2] = 1.0f;
	float x = pos[2 * i + 0];
	float y = pos[2 * i + 1];
	float p = (dens[i] - rest_density) * stiffness_constant;
	float curr_dens_inv = 1.0f / dens[i];
	float curr_dens_inv2 = curr_dens_inv * curr_dens_inv;
	float vx = vel[2 * i + 0];
	float vy = vel[2 * i + 1];
	float curvature = 0.0f;
	float normal_x = 0.0f;
	float normal_y = 0.0f;
	*ax = *ay = 0.0f;
	simp_list* list = simp_quadtree_query(qtree, x - h, y - h, x + h, y + h);
	simp_list_iter* iter = simp_list_iter_create(list);
	int j;
	while(simp_list_iter_next(iter, &j))
	{
		if(j == i) { continue; }
		float other_x = pos[2 * j + 0];
		float other_y = pos[2 * j + 1];
		float dx = other_x - x;
		float dy = other_y - y;
		float dd = dx * dx + dy * dy;
		//Check if the other point is contained inside the ball with radius h
		if(dd <= h * h)
		{
			float d = sqrtf(dd);
			float weight_grad = density_kernel_derivative(d, h);
			float j_dens_inv = 1.0f / dens[j];
			float p_other = (dens[j] - rest_density) * stiffness_constant;
			float c = weight_grad * (p * curr_dens_inv2 + p_other * j_dens_inv); 
			if(d < 1e-5)
			{
				frand2d(&dx, &dy);
			}
			else
			{
				float weight_surface = surface_tension_derivative(d, h) * j_dens_inv;
				normal_x += dx * weight_surface;
				normal_y += dy * weight_surface;
				dx /= d;
				dy /= d;
				curvature += surface_tension_laplacian(dd, h) * j_dens_inv; 
			}
			*ax += c * dx;
			*ay += c * dy;

			float viscosity_weight = viscosity_kernel_laplacian(d, h);
			c = viscosity_coefficient * viscosity_weight * j_dens_inv;
			*ax += c * (vel[2 * j + 0] - vx);
			*ay += c * (vel[2 * j + 1] - vy);
		}
	}
	float normal_d = sqrtf(normal_x * normal_x + normal_y * normal_y);
	//printf("%f\n", normal_d);
	if(normal_d > 2e-1)
	{
		normal_x /= normal_d;
		normal_y /= normal_d;
		float c = surface_coefficient * curvature;
		*ax += c * normal_x;
		*ay += c * normal_y;
		col[3 * i + 0] = 1.0f;
		col[3 * i + 1] = 0.0f;
		col[3 * i + 2] = 0.0f;
	}
	*ax /= dens[i];
	*ay /= dens[i];

	simp_list_iter_destroy(iter);
	simp_list_destroy(list);
}

static float density_kernel(float dst, float h)
{
	float volume = 0.1f * PI * pow(h, 5);
	float res = h - dst;
	return (res * res * res) / volume;
}

static float density_kernel_derivative(float dst, float h)
{
	float f2 = density_kernel(dst + 1e-6, h);
	float f1 = density_kernel(dst - 1e-6, h);
	return 0.5f * (f2 - f1) * 1e6;
	//float volume = 0.1f * PI * pow(h, 5);
	//float res = h - dst;
	//return (-3.0f * res * res) / volume;
}

static float viscosity_kernel_laplacian(float dst, float h)
{
	return (h - dst) * 40.0f / (PI * pow(h, 5));
}

static float surface_tension_derivative(float dst, float h)
{
	return -24.0f * dst * pow(h * h - dst * dst, 2) / (PI * pow(h, 8));
}

static float surface_tension_laplacian(float dd, float h)
{
	float hh = h * h;
	return -24.0f * (3.0f * hh * hh - 10.0f * hh * dd + 7.0f * dd * dd) / (PI * pow(h, 8));
}
