#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <threads/malloc.h>
#include <threads/palloc.h>
#include "filesys/file.h"
#include "vm/page.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "userprog/syscall.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "lib/kernel/list.h"

// Function prototypes
static bool vm_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED);
static unsigned vm_hash_func(const struct hash_elem *e, void *aux UNUSED);
static void vm_destroy_func(struct hash_elem *e, void *aux UNUSED);

// Compare function for hash_insert and hash_find
static bool vm_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
    struct vm_entry *vme_a = hash_entry(a, struct vm_entry, elem);
    struct vm_entry *vme_b = hash_entry(b, struct vm_entry, elem);

    return vme_a->vaddr < vme_b->vaddr;
}

// Hash function for virtual memory entries
static unsigned vm_hash_func(const struct hash_elem *e, void *aux UNUSED)
{
    struct vm_entry *vme = hash_entry(e, struct vm_entry, elem);
    return hash_int((int)vme->vaddr);
}

// Function to free resources associated with a VM entry
static void vm_destroy_func(struct hash_elem *e, void *aux UNUSED)
{
    struct vm_entry *vme = hash_entry(e, struct vm_entry, elem);
    void *physical_address;

    // If virtual address is loaded in physical memory
    if (vme->is_loaded)
    {
        // Get physical address and free page
        physical_address = pagedir_get_page(thread_current()->pagedir, vme->vaddr);
        palloc_free_page(physical_address);

        // Clear page table entry
        pagedir_clear_page(thread_current()->pagedir, vme->vaddr);
    }

    // Free VM entry
    free(vme);
}

// Initialize the virtual memory manager
void vm_init(struct hash *vm)
{
    hash_init(vm, vm_hash_func, vm_less_func, NULL);
}

// Destroy the virtual memory manager, freeing associated resources
void vm_destroy(struct hash *vm)
{
    hash_destroy(vm, vm_destroy_func);
}

// Find a VM entry based on a virtual address
struct vm_entry *find_vme(void *vaddr)
{
    struct vm_entry vme;
    struct hash_elem *element;

    // Try to find VM entry using hash_find
    vme.vaddr = pg_round_down(vaddr);
    element = hash_find(&thread_current()->vm, &vme.elem);

    // If found, return the VM entry
    if (element != NULL)
    {
        return hash_entry(element, struct vm_entry, elem);
    }

    return NULL;
}

// Insert a VM entry into the virtual memory manager
bool insert_vme(struct hash *vm, struct vm_entry *vme)
{
    // Return true if hash_insert is successful
    return hash_insert(vm, &vme->elem) == NULL;
}

// Delete a VM entry from the virtual memory manager
bool delete_vme(struct hash *vm, struct vm_entry *vme)
{
    // Return true if hash_delete is successful
    bool result = hash_delete(vm, &vme->elem) != NULL;

    // Free VM entry
    free(vme);

    return result;
}

// Load the contents of a file into physical memory at a given kernel address
bool load_file(void *kaddr, struct vm_entry *vme)
{
    bool result = false;

    // Read the file and return true if successful
    if ((int)vme->read_bytes == file_read_at(vme->file, kaddr, vme->read_bytes, vme->offset))
    {
        result = true;
        // Zero out remaining bytes
        memset(kaddr + vme->read_bytes, 0, vme->zero_bytes);
    }

    return result;
}
