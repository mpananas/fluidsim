#include "simp_quadtree.h"
#include <stdlib.h>

typedef struct node node;

typedef struct simp_quadtree
{
	float x0, y0, x1, y1;
	uint32_t resolution, count;
	uint32_t* bucket;
	float* points;
	simp_quadtree* children[4];
	bool split_flag;
}simp_quadtree;

struct node
{
	uint32_t val;
	node* next;
};

static void			__query(simp_quadtree* qtree, float x0, float y0, float x1,	float y1, simp_list* list);
static bool			__contains(float x1, float y1, float x2, float y2, float px, float py);
static bool			__intersects(float x11, float y11, float x12, float y12,
								 float x21, float y21, float x22, float y22);
static void			__split(simp_quadtree* qtree);

simp_quadtree*		simp_quadtree_create(float x0, float y0, float x1, float y1, uint32_t resolution)
{
	if(resolution < 1u) { resolution = 1u; }
	simp_quadtree* qtree = malloc(sizeof *qtree);
	if(!qtree) { goto QTREE_FAIL; }
	uint32_t* bucket = malloc(resolution * sizeof *bucket);
	if(!bucket) { goto BUCKET_FAIL; }
	float* points = malloc(resolution * 2u * sizeof *points);
	if(!points) { goto POINTS_FAIL; }

	qtree->x0 = x0;
	qtree->y0 = y0;
	qtree->x1 = x1;
	qtree->y1 = y1;
	qtree->resolution = resolution;
	qtree->count = 0u;
	qtree->bucket = bucket;
	qtree->points = points;
	qtree->split_flag = false;
	qtree->children[0] = qtree->children[1] = qtree->children[2] = qtree->children[3] = NULL;
	return qtree;

POINTS_FAIL:
	free(points);
BUCKET_FAIL:
	free(bucket);
QTREE_FAIL:
	free(qtree);
	return NULL;
}

void				simp_quadtree_destroy(simp_quadtree* qtree)
{
	if(!qtree) { return; }
	if(qtree->split_flag)
	{
		simp_quadtree_destroy(qtree->children[0]);
		simp_quadtree_destroy(qtree->children[1]);
		simp_quadtree_destroy(qtree->children[2]);
		simp_quadtree_destroy(qtree->children[3]);
	}
	free(qtree->bucket);
	free(qtree->points);
	free(qtree);
}

bool				simp_quadtree_insert(simp_quadtree* qtree, float x, float y, uint32_t index)
{
	if (!__contains(qtree->x0, qtree->y0, qtree->x1, qtree->y1, x, y)) { return false; }

	if (qtree->count < qtree->resolution)
	{
		qtree->bucket[qtree->count] = index;
		qtree->points[2 * qtree->count + 0] = x;
		qtree->points[2 * qtree->count + 1] = y;
		qtree->count++;
		return true;
	}
	else
	{
		if(!qtree->split_flag)
		{
			__split(qtree);
		}

		if( simp_quadtree_insert(qtree->children[0], x, y, index) ||
			simp_quadtree_insert(qtree->children[1], x, y, index) ||
			simp_quadtree_insert(qtree->children[2], x, y, index) ||
			simp_quadtree_insert(qtree->children[3], x, y, index)  )
		{
			return true;
		}
	}
	return true;
}

simp_list*			simp_quadtree_query(simp_quadtree* qtree, float x0, float y0, float x1, float y1)
{
	simp_list* list = simp_list_create(sizeof(uint32_t));
	__query(qtree, x0, y0, x1, y1, list);
	return list;
}



static void			__query(simp_quadtree* qtree, float x0, float y0, float x1,	float y1, simp_list* list)
{
	if(__intersects(qtree->x0, qtree->y0, qtree->x1, qtree->y1,
	x0, y0, x1, y1))
	{
		for(int i = 0; i < qtree->count; i++)
			if(__contains(x0, y0, x1, y1, qtree->points[2 * i + 0], qtree->points[2 * i + 1]))
				simp_list_push_tail(list, &qtree->bucket[i]);

		if(qtree->split_flag)
		{
			__query(qtree->children[0], x0, y0, x1, y1, list);
			__query(qtree->children[1], x0, y0, x1, y1, list);
			__query(qtree->children[2], x0, y0, x1, y1, list);
			__query(qtree->children[3], x0, y0, x1, y1, list);
		}
	}
}

static bool			__contains(float x1, float y1, float x2, float y2, float px, float py)
{
	return (
		(x1 <= px) &&
		(x2 >= px) &&
		(y1 <= py) &&
		(y2 >= py)
		);
}

static bool			__intersects(float x11, float y11, float x12, float y12, 
								 float x21, float y21, float x22, float y22)
{
	return !(
		(x12 <= x21) || 
		(x11 > x22) || 
		(y12 <= y21) || 
		(y11 > y22)
		);
}

static void			__split(simp_quadtree* qtree)
{
	float w = (qtree->x1 - qtree->x0) * 0.5f;
	float h = (qtree->y1 - qtree->y0) * 0.5f;
	float xc = qtree->x0 + w;
	float yc = qtree->y0 + h;
	qtree->children[0] = simp_quadtree_create(xc, yc - h, xc + w, yc, qtree->resolution);
	qtree->children[1] = simp_quadtree_create(xc - w, yc - h, xc, yc, qtree->resolution);
	qtree->children[2] = simp_quadtree_create(xc - w, yc, xc, yc + h, qtree->resolution);
	qtree->children[3] = simp_quadtree_create(xc, yc, xc + w, yc + h, qtree->resolution);
	qtree->split_flag = 1;
}
