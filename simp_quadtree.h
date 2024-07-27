#include "simp_list.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct simp_quadtree simp_quadtree;
typedef struct simp_qtree_list simp_qtree_list;

simp_quadtree*		simp_quadtree_create(float x0, float y0, float x1, float y1, uint32_t resolution);
void				simp_quadtree_destroy(simp_quadtree* qtree);
bool				simp_quadtree_insert(simp_quadtree* qtree, float x, float y, uint32_t index);
simp_list*			simp_quadtree_query(simp_quadtree* qtree, float x0, float y0, float x1, float y1);
bool				simp_qtree_list_next(simp_qtree_list* list, uint32_t* val);
void				simp_qtree_list_set(simp_qtree_list* list);
