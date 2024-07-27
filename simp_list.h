#include <stdint.h>
#include <stdbool.h>

typedef struct simp_list simp_list;
typedef struct simp_list_iter simp_list_iter;

simp_list*									simp_list_create(size_t bentry_size);
void										simp_list_destroy(simp_list* list);
void										simp_list_push_head(simp_list* list, void* entry);
void										simp_list_push_tail(simp_list* list, void* entry);
bool										simp_list_head(simp_list* list, void* entry);
bool										simp_list_tail(simp_list* list, void* entry);
void										simp_list_pop_head(simp_list* list);
void										simp_list_pop_tail(simp_list* list);
uint32_t									simp_list_size(simp_list* list);

simp_list_iter*								simp_list_iter_create(simp_list* list);
void										simp_list_iter_bind(simp_list_iter* iter, simp_list* list);
void										simp_list_iter_destroy(simp_list_iter* iter);
void										simp_list_iter_begin(simp_list_iter* iter);
void										simp_list_iter_end(simp_list_iter* iter);
bool										simp_list_iter_next(simp_list_iter* iter, void* entry);
bool										simp_list_iter_prev(simp_list_iter* iter, void* entry);
void										simp_list_iter_insert(simp_list_iter* iter, void* entry);
void										simp_list_iter_remove(simp_list_iter* iter);
