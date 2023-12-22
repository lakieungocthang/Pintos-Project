#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <hash.h>
#include <list.h>
#include "threads/synch.h"
#include "vm/page.h"
#include "threads/palloc.h"

void lru_list_init(void);
void add_page_to_lru_list(struct page* page);
void del_page_from_lru_list(struct page *page);

void try_to_free_pages(enum palloc_flags flags);
struct page *alloc_page(enum palloc_flags flags);
void free_page(void *kaddr);
void _free_page(struct page *page);

#endif