#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include <list.h>
#include "threads/thread.h"
#include "threads/vaddr.h"

#define VM_BIN 0    /* Load data from a binary file */
#define VM_FILE 1   /* Load data from a mapped file */
#define VM_ANON 2   /* Load data from the swap area */

struct vm_entry {
    uint8_t type;           /* Type: VM_BIN, VM_FILE, VM_ANON */
    void *vaddr;            /* Virtual page number managed by vm_entry */
    bool writable;          /* True if write is allowed at this address */
    bool pinned;            /* True if the entry is pinned (not swappable) */
    bool is_loaded;         /* Flag indicating whether the data is loaded in physical memory */
    struct file *file;      /* File mapped to the virtual address */

    /* For memory-mapped files (to be handled later) */
    struct list_elem mmap_elem;   /* List element for mmap */

    size_t offset;          /* Offset in the file to be read */
    size_t read_bytes;      /* Size of data written to the virtual page */
    size_t zero_bytes;      /* Remaining bytes to be zero-filled in the page */

    /* For swapping (to be handled later) */
    size_t swap_slot;       /* Swap slot */

    /* Data structures for vm_entry (to be handled later) */
    struct hash_elem elem;  /* Hash table element */
};

struct page {
    void *kaddr;            /* Physical address of the page */
    struct vm_entry *vme;   /* Pointer to the vm_entry representing the mapped virtual address */
    struct thread *thread;  /* Pointer to the thread using this physical page */
    struct list_elem lru;   /* List element for LRU list */
};

void vm_init(struct hash *vm);
bool insert_vme(struct hash *vm, struct vm_entry *vme);
bool delete_vme(struct hash *vm, struct vm_entry *vme);

struct vm_entry *find_vme(void *vaddr);
void vm_destroy(struct hash *vm);

bool load_file(void *kaddr, struct vm_entry *vme);

#endif /* VM_PAGE_H */
