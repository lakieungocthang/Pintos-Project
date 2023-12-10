#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>

#define VM_BIN 1 
#define VM_FILE 2
#define VM_ANON 3
#define CLOSE_ALL 9999

/* 
   Structure representing a virtual memory entry (vm_entry).
   Each entry corresponds to a specific region in the virtual memory.
*/
struct vm_entry {
    uint8_t type;                 // Type of the virtual memory entry (VM_BIN, VM_FILE, VM_ANON)
    void *vaddr;                  // Virtual address of the entry
    bool writable;                // Flag indicating if the entry is writable
    bool is_loaded;               // Flag indicating if the physical memory is loaded for this entry
    bool pinned;                  // Flag indicating if the entry is pinned in memory
    struct file *file;            // File associated with the entry (for VM_FILE type)
    struct list_elem mmap_elem;   // List element for mmap_file's vm_list
    size_t offset;                // Offset within the file (for VM_FILE type)
    size_t read_bytes;            // Number of bytes to read from the file
    size_t zero_bytes;            // Number of zero-initialized bytes
    size_t swap_slot;             // Swap slot for swapping pages in and out
    struct hash_elem elem;        // Hash element for the thread's virtual memory hash table
};

/*
   Initialize the virtual memory manager.
*/
void vm_init(struct hash *vm);

/*
   Destroy the virtual memory manager.
*/
void vm_destroy(struct hash *vm);

/*
   Find a virtual memory entry based on a given virtual address.
*/
struct vm_entry *find_vme(void *vaddr);

/*
   Insert a virtual memory entry into the virtual memory manager.
*/
bool insert_vme(struct hash *vm, struct vm_entry *vme);

/*
   Delete a virtual memory entry from the virtual memory manager.
*/
bool delete_vme(struct hash *vm, struct vm_entry *vme);

/*
   Load the contents of a file into physical memory at a given kernel address.
*/
bool load_file(void *kaddr, struct vm_entry *vme);

#endif
