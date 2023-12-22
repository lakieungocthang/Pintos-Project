#include "file.h"

struct list lru_list; // LRU list
struct lock lru_list_lock;
struct list_elem *lru_clock;

// Move the LRU clock to the next position in the clock algorithm
static struct list_elem *get_next_lru_clock() {
    struct list_elem *next_elem;

    // If the LRU list is empty
    if (list_empty(&lru_list))
        return NULL;

    // If the LRU clock is NULL or at the end of the list, set it to the beginning
    if (lru_clock == NULL || lru_clock == list_end(&lru_list))
        return list_begin(&lru_list);

    // If there is no next element, i.e., at the end of the list, set it to the beginning
    if (list_next(lru_clock) == list_end(&lru_list)) {
        return list_begin(&lru_list);
    } else {
        // If there is a next element, return it
        return list_next(lru_clock);
    }
}

// Initialize data structures related to LRU
void lru_list_init(void) {
    // Initialize the LRU list
    list_init(&lru_list);
    // Initialize the lock for the LRU list
    lock_init(&lru_list_lock);
    // Set the LRU clock to NULL
    lru_clock = NULL;
}

// Add a user page to the end of the LRU list
void add_page_to_lru_list(struct page *page) {
    list_push_back(&lru_list, &(page->lru));
}

// Remove a user page from the LRU list
void del_page_from_lru_list(struct page *page) {
    // If the page to be removed is pointed to by the LRU clock,
    // update the LRU clock to the next element
    if (&page->lru == lru_clock) {
        lru_clock = list_next(lru_clock);
    }
    list_remove(&page->lru);
}

// Try to free pages using the clock algorithm when facing a memory shortage
void try_to_free_pages(enum palloc_flags flags) {
    struct page *page;
    struct page *victim;

    lru_clock = get_next_lru_clock();

    // Choose a victim page using the clock algorithm
    page = list_entry(lru_clock, struct page, lru);

    // Iterate until a suitable victim page is found
    while (page->vme->pinned || pagedir_is_accessed(page->thread->pagedir, page->vme->vaddr)) {
        pagedir_set_accessed(page->thread->pagedir, page->vme->vaddr, false);
        lru_clock = get_next_lru_clock();
        page = list_entry(lru_clock, struct page, lru);
    }

    // Victim page identified
    victim = page;

    // Handle the victim based on its type (VM_BIN, VM_FILE, VM_ANON)
    switch (victim->vme->type) {
        case VM_BIN:
            if (pagedir_is_dirty(victim->thread->pagedir, victim->vme->vaddr)) {
                victim->vme->swap_slot = swap_out(victim->kaddr);
                victim->vme->type = VM_ANON;
            }
            break;
        case VM_FILE:
            if (pagedir_is_dirty(victim->thread->pagedir, victim->vme->vaddr)) {
                file_write_at(victim->vme->file, victim->vme->vaddr, victim->vme->read_bytes, victim->vme->offset);
            }
            break;
        case VM_ANON:
            victim->vme->swap_slot = swap_out(victim->kaddr);
            break;
    }

    // Mark the page as not loaded in memory
    victim->vme->is_loaded = false;

    // Free the victim page
    _free_page(victim);
}

// Allocate a new physical page
struct page *alloc_page(enum palloc_flags flags) {
    lock_acquire(&lru_list_lock);

    // Allocate a new page using palloc_get_page()
    uint8_t *kpage = palloc_get_page(flags);

    // Retry allocation until successful
    while (kpage == NULL) {
        try_to_free_pages(flags);
        kpage = palloc_get_page(flags);
    }

    // Create and initialize a new page structure
    struct page *page = malloc(sizeof(struct page));
    page->kaddr = kpage;
    page->thread = thread_current();

    // Add the page to the LRU list
    add_page_to_lru_list(page);

    lock_release(&lru_list_lock);

    return page;
}

// Free a physical page
void free_page(void *kaddr) {
    lock_acquire(&lru_list_lock);

    // Find the page in the LRU list using its kernel address
    struct list_elem *e;
    struct page *page = NULL;

    for (e = list_begin(&lru_list); e != list_end(&lru_list); e = list_next(e)) {
        struct page *candidate_page = list_entry(e, struct page, lru);
        if (candidate_page->kaddr == kaddr) {
            page = candidate_page;
            break;
        }
    }

    // If the page is found, free it
    if (page != NULL)
        _free_page(page);

    lock_release(&lru_list_lock);
}

// Internal function to free a physical page
void _free_page(struct page *page) {
    // Remove the page from the LRU list
    del_page_from_lru_list(page);

    // Clear the page table entry and free the allocated memory
    pagedir_clear_page(page->thread->pagedir, pg_round_down(page->vme->vaddr));
    palloc_free_page(page->kaddr);
    free(page);
}
