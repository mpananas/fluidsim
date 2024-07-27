#include "simp_list.h"
#include <stdlib.h>
#include <string.h>

typedef struct node node;

typedef struct simp_list
{
	uint32_t size;
	size_t bentry_size;
	node *head, *tail;
}simp_list;

typedef struct simp_list_iter
{
	simp_list* list;
	node* curr;
}simp_list_iter;

struct node
{
	node *next, *prev;
};

static node*								__create_node(void* entry, size_t bentry_size);

simp_list*									simp_list_create(size_t bentry_size)
{
	simp_list* list = malloc(sizeof *list);
	if(!list || bentry_size == 0u) { return NULL; }
	list->head = list->tail = NULL;
	list->size = 0u;
	list->bentry_size = bentry_size;
	return list;
}

void										simp_list_destroy(simp_list* list)
{
	if(list->size == 0u) { return; }
	node* curr_node = list->head, *next_node;
	while(curr_node)
	{
		next_node = curr_node->next;
		free(curr_node);
		curr_node = next_node;
	}
	free(list);
}

void										simp_list_push_head(simp_list* list, void* entry)
{
	node* new_node = __create_node(entry, list->bentry_size);
	if(!new_node) { return; }
	if(list->size++ == 0u)
	{
		list->head = list->tail = new_node;
	}
	else
	{
		new_node->next = list->head;
		list->head->prev = new_node;
		list->head = new_node;
	}
}

void										simp_list_push_tail(simp_list* list, void* entry)
{
	node* new_node = __create_node(entry, list->bentry_size);
	if(!new_node) { return; }
	if(list->size++ == 0u)
	{
		list->head = list->tail = new_node;
	}
	else
	{
		new_node->prev = list->tail;
		list->tail->next = new_node;
		list->tail = new_node;
	}
}

bool										simp_list_head(simp_list* list, void* entry)
{
	if(list->size == 0u) { return false; }
	memcpy(entry, (uint8_t*)list->head + sizeof *list->head, list->bentry_size);
	return true;
}

bool										simp_list_tail(simp_list* list, void* entry)
{
	if(list->size == 0u) { return false; }
	memcpy(entry, (uint8_t*)list->tail + sizeof *list->tail, list->bentry_size);
	return true;
}

void										simp_list_pop_head(simp_list* list)
{
	if(list->size == 0u) { return; }
	node* curr_node = list->head;
	list->head = list->head->next;
	list->head->prev = NULL;
	list->size--;
	free(curr_node);
}

void										simp_list_pop_tail(simp_list* list)
{
	if(list->size == 0u) { return; }
	node* curr_node = list->tail;
	list->tail = list->tail->prev;
	list->tail->next = NULL;
	list->size--;
	free(curr_node);
}

uint32_t									simp_list_size(simp_list* list)
{
	return list->size;
}


simp_list_iter*								simp_list_iter_create(simp_list* list)
{
	simp_list_iter* iter = malloc(sizeof *iter);
	if(!iter) { return NULL; }
	iter->list = list;
	iter->curr = list->head;
	return iter;
}

void										simp_list_iter_bind(simp_list_iter* iter, simp_list* list)
{
	iter->list = list;
	iter->curr = list->head;
}

void										simp_list_iter_destroy(simp_list_iter* iter)
{
	free(iter);
}

void										simp_list_iter_begin(simp_list_iter* iter)
{
	iter->curr = iter->list->head;
}

void										simp_list_iter_end(simp_list_iter* iter)
{
	iter->curr = iter->list->tail;
}

bool										simp_list_iter_next(simp_list_iter* iter, void* entry)
{
	if(!iter->curr) { return false; }
	if(entry)
		memcpy(entry, (uint8_t*)iter->curr + sizeof *iter->curr, iter->list->bentry_size);
	iter->curr = iter->curr->next;
	return true;
}

bool										simp_list_iter_prev(simp_list_iter* iter, void* entry)
{
	if(!iter->curr) { return false; }
	if(entry)
		memcpy(entry, (uint8_t*)iter->curr + sizeof *iter->curr, iter->list->bentry_size);
	iter->curr = iter->curr->prev;
	return true;
}

void										simp_list_iter_insert(simp_list_iter* iter, void* entry)
{
	if(iter->list->size == 0u)
	{
		simp_list_push_head(iter->list, entry);
		iter->curr = iter->list->head;
	}
	else
	{
		if(!iter->curr) { return; }
		node* new_node = __create_node(entry, iter->list->bentry_size);
		node* prev = iter->curr, *next = iter->curr->next;
		if(iter->curr == iter->list->tail)
		{
			simp_list_push_tail(iter->list, entry);
		}
		else
		{
			prev->next = new_node;
			if(next)
			{
				next->prev = new_node;
				new_node->next = next;
			}
			new_node->prev = prev;
			iter->list->size++;
		}
	}
}

void										simp_list_iter_remove(simp_list_iter* iter)
{
	if(iter->list->size == 0u || !iter->curr) { return; }
	if(iter->curr == iter->list->head || iter->list->size == 1u)
	{
		simp_list_pop_head(iter->list);
		iter->curr = iter->list->head;
	}
	else if(iter->curr == iter->list->tail)
	{
		simp_list_pop_tail(iter->list);
		iter->curr = iter->list->tail;
	}
	else
	{
		node* curr = iter->curr;
		node* prev = iter->curr->prev, *next = iter->curr->next;
		prev->next = next;
		next->prev = prev;
		free(curr);
		iter->curr = next;
		iter->list->size--;
	}
}


static node*								__create_node(void* entry, size_t bentry_size)
{
	node* p = malloc(sizeof *p + bentry_size);
	if(!p) { return NULL; }
	p->next = p->prev = NULL;
	memcpy((unsigned char*)(p) + sizeof *p, entry, bentry_size);
	return p;
}
//info@trustserv.gr
