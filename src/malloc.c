/* MALLOC: implementation of memory allocator using mmap */

#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>

// EXAMPLE:
/* #define PROT_READ  (1 << 0)  // 0000 0001 */
/* #define PROT_WRITE (1 << 1)  // 0000 0010 */
/* #define PROT_EXEC  (1 << 2)  // 0000 0100 */
/* PROT_READ | PROT_WRITE       // 0000 0011 */

// Minimum size of a block of memory in the heap.
#define MIN_BLOCK_SIZE sizeof(size_t)

typedef enum {
  false = 0,
  true  = 1,
} bool;

/* Block of memory in the heap.
     NOTE: the blocks are contiguous. So technically we don't need a
     pointer to the next block, as we could infer it by adding the size.
     NOTE: size_t indicates word size. */
typedef struct Block {
  size_t size;  // Space available for allocating actual data.
  bool   free;
  struct Block* next;
} Block;

// Heap structure.
typedef struct {
  // Head of heap linked list and beginning of the heap.
  Block* head;
  size_t size;
} Heap;

// Global heap instance.
Heap heap;

// Initialize the heap.
void init_heap(size_t size) {
  // Allocate some memory for the heap.
  void* memory = mmap(0, size, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANON, -1, 0);

  // Initialize the heap structure.
  heap.head = memory;
  heap.size = size;

  // Get the address of the first block and initialize it.
  Block* first = heap.head;
  // Available size does not include the inlined Block struct.
  first->size = size - sizeof(Block);
  first->free = true;
  first->next = NULL;
}

// Find a big enough block.
Block* find_free_block(size_t size) {
  Block* curr = heap.head;
  while (curr != NULL) {
    if (curr->free && curr->size >= size) {
      return curr;
    }
    curr = curr->next;
  }
  return NULL;
}

void split_block(Block* block, size_t left_size) {
  // Create another block to the right.
  // NOTE: must cast block to void* (C is fucked).
  Block* new_block = (void*) block + sizeof(Block) + left_size;
  new_block->size = block->size - sizeof(Block) - left_size;
  new_block->free = true;

  // Change the size of the original block (shrink).
  block->size = left_size;

  // Add the new block to the linked list.
  new_block->next = block->next;
  block->next = new_block;
}

// Allocate memory.
void* malloc(size_t size) {
  // Find a big enough block.
  Block* block = find_free_block(size);

  // If we couldn't find a block big enough, return NULL.
  if (block == NULL) {
    return NULL;
  }

  // If there's enough space for another block of free memory,
  // split the current one.
  if (block->size >= (size + sizeof(Block) + MIN_BLOCK_SIZE)) {
    split_block(block, size);
  }
  // Set the block as busy.
  block->free = false;

  return (void*) block + sizeof(Block);
}

void print_heap(Heap* heap) {
  // FREE start: 0x8000000, size: 0x1000
  // BUSY start: 0x8002000, size: 0x9000

  Block* curr = heap->head;
  while (curr != NULL) {
    printf("%s start: 0x%zX, size: 0x%zX\n",
           curr->free ? "FREE" : "BUSY",
           (size_t) curr,
           curr->size);
    curr = curr->next;
  }
}

int main(int argc, char* argv[]) {
  init_heap(0x10000);
  print_heap(&heap);
  printf("\n");

  // an example
  int* some_int = malloc(sizeof(int));
  int* another_int = malloc(sizeof(int));

  *some_int = 3;
  print_heap(&heap);
  printf("\n");
  *another_int = 7;

  printf("%d, %d\n\n", *some_int, *another_int);;
  print_heap(&heap);

  // TODO: free examples

  return 0;
}
