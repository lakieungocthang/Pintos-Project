#include "vm/swap.h"
#include "devices/block.h"
#include "vm/file.h"
#include "vm/page.h"

const size_t BLOCKS_PER_PAGE = PGSIZE / BLOCK_SECTOR_SIZE; // Number of blocks per page

struct bitmap *swap_bitmap; // Bitmap indicating whether a particular index in the swap area is in use

// Initialize the swap area
void swap_init() {
    swap_bitmap = bitmap_create(8 * 1024);
}

// Copy data from the swap slot at used_index to the logical address kaddr
void swap_in(size_t used_index, void *kaddr) {
    struct block *swap_disk = block_get_role(BLOCK_SWAP);
    
    if (bitmap_test(swap_bitmap, used_index)) {
        int i;
        for (i = 0; i < BLOCKS_PER_PAGE; i++) {
            // Calculate the block sector to read and copy to the physical page
            block_read(swap_disk, BLOCKS_PER_PAGE * used_index + i, BLOCK_SECTOR_SIZE * i + kaddr);
        }
        bitmap_reset(swap_bitmap, used_index);
    }
}

// Write the contents of the page pointed to by kaddr to the swap partition
size_t swap_out(void *kaddr) {
    struct block *swap_disk = block_get_role(BLOCK_SWAP);

    // Find the first false bit index using first-fit strategy
    size_t swap_index = bitmap_scan(swap_bitmap, 0, 1, false);

    if (BITMAP_ERROR != swap_index) {
        int i;
        for (i = 0; i < BLOCKS_PER_PAGE; i++) {
            // Calculate the block sector to write from the physical page to the swap area
            block_write(swap_disk, BLOCKS_PER_PAGE * swap_index + i, BLOCK_SECTOR_SIZE * i + kaddr);
        }
        bitmap_set(swap_bitmap, swap_index, true);
    }

    return swap_index;
}
