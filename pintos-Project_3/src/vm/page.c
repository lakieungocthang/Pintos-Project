#include "vm/page.h"
#include "vm/file.h"
#include "vm/swap.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"
#include "filesys/file.h"
#include "threads/interrupt.h"
#include <string.h>

// Hash function for vm_entry's vaddr using hash_int()
static unsigned vm_hash_func(const struct hash_elem *e, void *aux) {
    struct vm_entry *vme = hash_entry(e, struct vm_entry, elem);
    return hash_int(vme->vaddr);
}

// Comparison function for vm_entry's vaddr values
static bool vm_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux) {
    struct vm_entry *vme_a = hash_entry(a, struct vm_entry, elem);
    struct vm_entry *vme_b = hash_entry(b, struct vm_entry, elem);
    return vme_a->vaddr < vme_b->vaddr;
}

// Destruction function for vm_entry elements in the hash table
static void vm_destroy_func(struct hash_elem *e, void *aux) {
    struct vm_entry *vme = hash_entry(e, struct vm_entry, elem);
    free_page(pagedir_get_page(thread_current()->pagedir, vme->vaddr));
    free(vme);
}

// Initialize the virtual memory hash table
void vm_init(struct hash *vm) {
    hash_init(vm, vm_hash_func, vm_less_func, NULL);
}

// Insert a vm_entry into the virtual memory hash table
bool insert_vme(struct hash *vm, struct vm_entry *vme) {
    vme->pinned = false;
    struct hash_elem *elem = hash_insert(vm, &(vme->elem));
    // hash_insert returns null on success
    return elem == NULL;
}

// Delete a vm_entry from the virtual memory hash table
bool delete_vme(struct hash *vm, struct vm_entry *vme) {
    struct hash_elem *elem = hash_delete(vm, &(vme->elem));
    // hash_delete returns null if the element was not found
    if (elem == NULL)
        return false;
    else {
        free_page(pagedir_get_page(thread_current()->pagedir, vme->vaddr));
        free(vme);
        return true;
    }
}

// Find a vm_entry based on the virtual address
struct vm_entry *find_vme(void *vaddr) {
    struct thread *cur = thread_current();
    struct vm_entry search_entry;

    // Round down vaddr to get the page number
    search_entry.vaddr = pg_round_down(vaddr);

    struct hash_elem *e = hash_find(&(cur->vm), &(search_entry.elem));

    // If the element is found, return the corresponding vm_entry
    if (e != NULL)
        return hash_entry(e, struct vm_entry, elem);
    else
        return NULL;
}

// Destroy the virtual memory hash table, freeing allocated memory
void vm_destroy(struct hash *vm) {
    hash_destroy(vm, vm_destroy_func);
}

// Load data from a file into physical memory
bool load_file(void *kaddr, struct vm_entry *vme) {
    // Set the file offset to the vm_entry's offset
    file_seek(vme->file, vme->offset);
    
    // Read data from the file into the physical page
    if (file_read(vme->file, kaddr, vme->read_bytes) != (int)(vme->read_bytes)) {
        return false;
    }

    // Pad the remaining zero_bytes with zeros
    memset(kaddr + vme->read_bytes, 0, vme->zero_bytes);

    // Return true if file_read is successful
    return true;
}
