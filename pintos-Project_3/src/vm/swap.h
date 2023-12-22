#ifndef VM_SWAP_H
#define VM_SWAP_H

#include <hash.h>
#include <list.h>
#include <bitmap.h>

void swap_init();
void swap_in(size_t used_index, void *kaddr);
size_t swap_out(void *kaddr);

#endif