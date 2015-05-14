/*
 * mm.c - fast, segregated, best-fit malloc package.
 * 
 * In this approach, we organize lists of free blocks of particular sizes in specific pointers.
 * We'll have access to the 'root' of each of these list of blocks, each of them connected explicitly
 * with pointers pointing to each other, so that we will be able to find the most appropriate
 * block each time we call malloc.
 * 
 * The method for finding each of the blocks is simple. It first looks at the list of blocks
 * with the range of size the allocating block has. It searches the list, and finds the most
 * optimal free block (smallest but larger than the block). If it fails, it looks in larger
 * ranges. If it finds none, we'll have to extend the heap.
 * 
 * This approach is pretty fast, since it knows right away where to look for, and also quite
 * efficient as it finds a block that is just a bit larger than the required size.
 * 
 * It uses 'segregate' and 'unsegregate' functions for putting the free blocks in and out of
 * the lists. 'segblock' function is used for finding which list the block belongs to.
 * It also supports coalescing, placing, as well as extending the heap.
 * 
 * Each of the blocks look like following:
 * (Allocated block)
 * | HDRP | (BLOCK) | FTRP |
 * 
 * (Free block: PREV_FREE is optional - not allowed for the smallest blocks)
 * | HDRP | NEXT_FREE_ADDR | (PREV_FREE_ADDR) | (BLOCK) | FTRP |
 * 
 * Descriptions for each components are written where they are defined. Please refer to them
 * for more detail
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your information in the following struct.
 ********************************************************/
team_t team = {
  /* Team name : Your student ID */
  "2013-11406",
  /* Your full name */
  "Won Wook SONG",
  /* Your student ID */
  "2013-11406",
  /* leave blank */
  "",
  /* leave blank */
  ""
};

/* DON'T MODIFY THIS VALUE AND LEAVE IT AS IT WAS */
static range_t **gl_ranges;

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))


/* MACROS FROM THE BOOK */
#define WSIZE 4 // Single wods size in bytes
#define DSIZE 8 // Double word size in bytes
#define CHUNKSIZE (1<<4) //extend the heap by this amount

#define MAX(x, y)   ((x) > (y) ? (x) : (y)) //MAX

/* Pack a size and allocated bit into a word (for header/footer) */
#define PACK(size, alloc)   ((size)|(alloc)) //PACKs size and alloc

/* Read and write a word at address p */
#define GET(p)      (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and the allocated bit from address p (p must be header/footer) */
#define GET_SIZE(p)   (GET(p) & ~0x7)
#define GET_ALLOC(p)  (GET(p) & 0x1)
#define GET_ALLOC_CHAR(p) ((GET_ALLOC(p)) ? 'A' : 'F' )

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)    ((char *)(bp) - WSIZE)
#define FTRP(bp)    ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of its next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE((HDRP(bp))))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

static char *heap_listp = 0;


/* ALL FUNCTIONS */
int mm_init(range_t **ranges);
void* mm_malloc(size_t size);
void mm_free(void *ptr);
void* mm_realloc(void *ptr, size_t t);
void mm_exit(void);

int mm_check(void);
void printheap(void);
void printlist(void);

static void *segblock(size_t asize);
static void segregate(void *bp);
static void unsegregate(void *bp);

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);

/* POINTERS TO EACH SEGLIST */
static char *list1; //2**4: 16,   24,   32,   40,   48,   56
static char *list2; //2**6: 64,   96,   128,  160,  192,  224
static char *list3; //2**8: 256,  384,  512,  640,  768,  896
static char *list4; //2**10:1024, 1536, 2048, 2560, 3072, 3584
static char *list5; //2**12:4096, 6144, 8192, 10240,12288,14336
static char *list6; //2**14:16384,24576,32768,40960,49152,else
static char *lastblock = 0;

/* OWN MACROS */
//#define DEBUG //for debugging
//#define DEBUG_LIST

#define BLK_SIZE(bp) (GET_SIZE(HDRP(bp))) //gets the size of bp
#define BLK_ALLOC(bp) (GET_ALLOC(HDRP(bp))) // gets the alloc of bp
#define BLK_ALLOC_CHAR(bp) ((BLK_ALLOC(bp)) ? 'A' : 'F') // returns the alloc of bp in 'A' or 'F'

// it shows the value of each beginning of each sets of lists.
#define SIZE1 16
#define SIZE2 64
#define SIZE3 256
#define SIZE4 1024
#define SIZE5 4096
#define SIZE6 16384
#define SIZE7 65536

#define LIST_PTR(lp, num) ((char *)(lp) + (num * (WSIZE))) //use is as LIST_PTR(list1, 1) to get list ptr to blocks of 16 - it returns the pointer to each lists
#define ROOT_ADDR(lp) (lp) //address of the root block of the list
#define ROOT_BLK(lp) (*(char *)(lp)) //returns the root block of the list
#define LIST_ALLOC(lp) (((lp) == NULL) ? 0 : 1) // sees if this seglist block is free (if there are any free blocks or not)
#define LIST_ALLOC_CHAR(lp) (((lp) == NULL) ? 'F' : 'A') // returns it with a 'F' or 'A'

#define GET_ROOT(bp) (GET(HDRP(bp)) & 0x4) // sees if the free block is seglist ROOT
#define GET_TAIL(bp) (GET(HDRP(bp)) & 0x2) // sees if the free block is seglist TAIL

#define FREEPACK(size, root, tail, alloc) ((size)|(root)|(tail)|(alloc)) // packs information for the header/footer of a free block

#define NEXT_FREE_ADDR(bp) (*(char **)(bp)) // gets address of next free block on the list
#define PREV_FREE_ADDR(bp) (*((char **)(bp) + WSIZE)) // gets address of previous free block on the list
#define NOPREVFREE 40 // the minimum size of the block that can have PREV_FREE_ADDR on the block - 40 to be safe


/* 
 * remove_range - manipulate range lists
 * DON'T MODIFY THIS FUNCTION AND LEAVE IT AS IT WAS
 */
static void remove_range(range_t **ranges, char *lo)
{
  range_t *p;
  range_t **prevpp = ranges;
  
  if (!ranges)
    return;

  for (p = *ranges;  p != NULL; p = p->next) {
    if (p->lo == lo) {
      *prevpp = p->next;
      free(p);
      break;
    }
    prevpp = &(p->next);
  }
}

/*
 * mm_init - initialize the malloc package (heap) and the seglist pointers
 */
int mm_init(range_t **ranges)
{
#ifdef DEBUG
printf("\n\n| mm_init BEGIN\n");
#endif
  /* Create the initial empty heap */
  int i;
  char **listblock;

  if ((heap_listp = mem_sbrk(36*WSIZE + 4*WSIZE)) == (void *)-1)
    return -1;

  /* make the seglist */
  list1 = heap_listp;
  list2 = (heap_listp += 6*WSIZE);
  list3 = (heap_listp += 6*WSIZE);
  list4 = (heap_listp += 6*WSIZE);
  list5 = (heap_listp += 6*WSIZE);
  list6 = (heap_listp += 6*WSIZE);
  heap_listp += 6*WSIZE;
  for(i=0; i<36; i++) {
    listblock = LIST_PTR(list1, i);
    *listblock = NULL;//initialize each seglist : set ROOT_ADDR as 0
#ifdef DEBUG
/*printf(" | initialized %p to %p | ", listblock, *listblock);*/
#endif
  }

  /* empty heap */
  PUT(heap_listp, 0);
  PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); //prologue block
  PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));
  PUT(heap_listp + (3*WSIZE), PACK(0, 1)); //epilogue block
  heap_listp += (2*WSIZE); // to the prologue block
  lastblock = heap_listp + WSIZE; // to the epilogue block

#ifdef DEBUG
mm_check();
#endif

  /* Extend the empty heap with a free block of CHUNKSIZE bytes */
  if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
    return -1;

  /* DON't MODIFY THIS STAGE AND LEAVE IT AS IT WAS */
  gl_ranges = ranges;

#ifdef DEBUG
printf("| init DONE\n");
#endif
#ifdef DEBUG
mm_check();
#endif

  return 0;
}

/*
 * mm_malloc - Allocate a block by finding an adequate free block.
 * Extend the heap if no such block exists.
 * Note that it only extends the amount it HAS to extend by adding up 
 * the size of the free block just before the epilogue block.
 * It always allocates a block whose size is a multiple of the alignment.
 */
void* mm_malloc(size_t size)
{
#ifdef DEBUG
printf("| mm_malloc BEGIN ");
#endif

  size_t asize;
  size_t extendsize = 0;
  char *bp;

  if (heap_listp == 0) 
    mm_init(NULL); // if heap doesn't exist, init.

  if (size == 0) return NULL; // ignore useless calls

  if (size <= DSIZE) asize = 3*DSIZE; // header, footer, next_free_addr
  else asize = DSIZE * ((size + DSIZE + (DSIZE-1)) / DSIZE);

#ifdef DEBUG
printf(" -- size: %zu, adjusted size: %zu \n", size, asize);
#endif

  if((bp = find_fit(asize)) != NULL) { // if an appropriate free block exists
    place(bp, asize);

#ifdef DEBUG
printf("| mm_malloc DONE!\n");
#endif

    return bp;
  }

  // an appropriate free block wasn't found
  if(!(BLK_ALLOC(PREV_BLKP(lastblock)))) // if the block before the epilogue block is free, we can remove its size from which we will have to extend
    extendsize = BLK_SIZE(PREV_BLKP(lastblock));
  extendsize = MAX(asize - extendsize, CHUNKSIZE);
  if((bp = extend_heap(extendsize/WSIZE)) == NULL) return NULL;
  place(bp, asize); // place it now. It will have to have an adequate block

#ifdef DEBUG
printf("| mm_malloc DONE\n");
#endif

  return bp;
}

/*
 * mm_free - Freeing a block. Coalesce accordingly with previous and next blocks.
 */
void mm_free(void *ptr)
{
  /* YOUR IMPLEMENTATION */
#ifdef DEBUG
printf("| mm_free BEGIN at:%p(%d)\n", ptr, BLK_SIZE(ptr));
#endif
  size_t size = BLK_SIZE(ptr);

  PUT(HDRP(ptr), PACK(size, 0));
  PUT(FTRP(ptr), PACK(size, 0));
  coalesce(ptr);

  /* DON't MODIFY THIS STAGE AND LEAVE IT AS IT WAS */
  if (gl_ranges)
    remove_range(gl_ranges, ptr);

#ifdef DEBUG
printf("| mm_free DONE\n");
mm_check();
#endif

  return;
}

/*
 * mm_realloc - empty implementation; YOU DO NOT NEED TO IMPLEMENT THIS
 */
void* mm_realloc(void *ptr, size_t t)
{
  return NULL;
}

/*
 * mm_exit - finalize the malloc package. Free all allocated blocks.
 */
void mm_exit(void)
{
#ifdef DEBUG
printf("| mm_exit BEGIN\n");
#endif
  void *bp = heap_listp;
  size_t totalsize = 0;

  for(bp = NEXT_BLKP(bp); BLK_SIZE(bp) > 0; bp = NEXT_BLKP(bp)) {
    if(BLK_ALLOC(bp)) {
      mm_free(bp);
    }
  }
#ifdef DEBUG
printf("| mm_exit_DONE\n");
mm_check();
#endif
  return;
}

/* ===============================EXTRA FUNCTIONS========================= */
/* ===============================  FOR TESTING  ========================= */
/*
 * mm_check - checks heap consistency : prints FATAL on inconsistency errors.
 * !!This is not all of the debugging process!! check the code inside #ifdef and #endif.
 * There are various tools that you can use by enabling #define DEBUG and #define DEBUG_LIST.
 * this function checks heap and list consistency.
 * 
 */
int mm_check(void) {
  printheap(); // prints the heap
#ifdef DEBUG_LIST 
  printlist(); // prints the lists
#endif
  return 0;
}

/*
 * printheap - prints out the entire heap with each block's information. we can check if init/malloc/free works well.
 */
void printheap(void) {
  printf("\tCURRENT HEAP: ");
  void *bp = 0;
  size_t size;
  int allocflag = 0;

  for(bp = heap_listp; BLK_SIZE(bp) > 0; bp = NEXT_BLKP(bp)) {
    size = (BLK_SIZE(bp));
    printf("| %p:%zu%2c |", bp, size, BLK_ALLOC_CHAR(bp));
    if(*HDRP(bp) != *FTRP(bp)) printf("==FATAL: HDR AND FTR DOESN'T MATCH: %p==", bp); // checks if the header and footer matches each other
    if(!(allocflag || BLK_ALLOC(bp))) printf("==FATAL: FREE BLOCKS HAVEN'T BEEN COALESCED: %p==", bp); // checks if the heap doesn't have consecutive free blocks
    allocflag = BLK_ALLOC(bp);
  }
  size = (BLK_SIZE(bp));
  printf("| %p:%zu%2c |\n", bp, size, BLK_ALLOC_CHAR(bp)); // epilogue block
  return;
}

/*
 * printlist - prints out the sizes and the root blocks of each seglists. We can check segregation here.
 */
void printlist(void) {
  printf("\tCURRENT LIST: ");
  char **listblock;
  char *bp=0;
  int i;

  listblock = list1;
  for(i=0; listblock < list2; listblock = LIST_PTR(list1, ++i)) { //prints list1
    printf("| %p:%5d->%p |", listblock, SIZE1 + (i * (SIZE1 / 2)), *listblock);
  } printf("\n\t              ");

  for(i=0; listblock < list3; listblock = LIST_PTR(list2, ++i)) { //prints list2
    printf("| %p:%5d->%p |", listblock, SIZE2 + (i * (SIZE2 / 2)), *listblock);
  } printf("\n\t              ");

  for(i=0; listblock < list4; listblock = LIST_PTR(list3, ++i)) { //prints list3
    printf("| %p:%5d->%p |", listblock, SIZE3 + (i * (SIZE3 / 2)), *listblock);
  } printf("\n\t              ");

  for(i=0; listblock < list5; listblock = LIST_PTR(list4, ++i)) { //prints list4
    printf("| %p:%5d->%p |", listblock, SIZE4 + (i * (SIZE4 / 2)), *listblock);
  } printf("\n\t              ");

  for(i=0; listblock < list6; listblock = LIST_PTR(list5, ++i)) { //prints list5
    printf("| %p:%5d->%p |", listblock, SIZE5 + (i * (SIZE5 / 2)), *listblock);
  } printf("\n\t              ");

  for(i=0; listblock < list6 + 6 * WSIZE; listblock = LIST_PTR(list6, ++i)) { //prints list6
    printf("| %p:%5d->%p |", listblock, SIZE6 + (i * (SIZE6 / 2)), *listblock);
  } printf("\n");
}

/* ===============================EXTRA FUNCTIONS========================= */

/*
 * segblock - returns the seglist block that has the range in which the input size fits in.
 * refer to where list1..list6 is defined to check out the ranges.
 */
static void *segblock(size_t asize) {
  int i;
  char *result;
  if(asize < SIZE2) {
    for(i=5; asize < SIZE1+(i*(SIZE1/2)); --i);
    result = LIST_PTR(list1, i);
  } else if(asize < SIZE3) {
    for(i=5; asize < SIZE2+(i*(SIZE2/2)); --i);
    result = LIST_PTR(list2, i);
  } else if(asize < SIZE4) {
    for(i=5; asize < SIZE3+(i*(SIZE3/2)); --i);
    result = LIST_PTR(list3, i);
  } else if(asize < SIZE5) {
    for(i=5; asize < SIZE4+(i*(SIZE4/2)); --i);
    result = LIST_PTR(list4, i);
  } else if(asize < SIZE6) {
    for(i=5; asize < SIZE5+(i*(SIZE5/2)); --i);
    result = LIST_PTR(list5, i);
  } else if(asize < SIZE7) {
    for(i=5; asize < SIZE6+(i*(SIZE6/2)); --i);
    result = LIST_PTR(list6, i);
  } else {
    result = LIST_PTR(list6, 5); //last one
  }
  return result;
}

/*
 * segregate - puts the block in a appropriate seglist and put NEXT_FREE_ADDR and PREV_FREE_ADDR accordingly.
 * segregate on an empty list: set the block as root and tail and attach it on the list
 * segregate on an nonempty list: set the block as a new root (push) and reset the original root.
 */
static void segregate(void *bp) {
  size_t size = BLK_SIZE(bp);
  char **listblock = (segblock(size)); // get where to put this free block

#ifdef DEBUG_LIST
printf("| | | | segregate BEGIN for %p(%d=%d) to %p:%c", bp, BLK_SIZE(bp), GET_SIZE(FTRP(bp)), listblock, LIST_ALLOC_CHAR(*listblock));
if(BLK_ALLOC(bp)) printf("  BLOCK IS ALLOCATED BUT TRYING TO SEGREGATE!!");
#endif

  if(*listblock == NULL) { //if seglist is empty
    *listblock = bp;
    if(BLK_SIZE(bp) > NOPREVFREE) PREV_FREE_ADDR(bp) = NULL;
    NEXT_FREE_ADDR(bp) = NULL;
    PUT(HDRP(bp), FREEPACK(size, 4, 2, 0));
    PUT(FTRP(bp), FREEPACK(size, 4, 2, 0));
#ifdef DEBUG_LIST
printf(" -- seglist is empty! segregate DONE: %p, root: %d, tail: %d\n", bp, GET_ROOT(bp), GET_TAIL(bp));
#endif
    return;
  }

  // seglist is not empty == setlist has a root
  char *oldroot;
  oldroot = *listblock; // get the old root
  *listblock = bp; //push the new root onto the seglist
  PUT(HDRP(oldroot), FREEPACK(BLK_SIZE(oldroot), 0, GET_TAIL(oldroot), 0)); // not root anymore
  PUT(FTRP(oldroot), FREEPACK(BLK_SIZE(oldroot), 0, GET_TAIL(oldroot), 0));
  PUT(HDRP(bp), FREEPACK(size, 4, 0, 0)); //bp as new root
  PUT(FTRP(bp), FREEPACK(size, 4, 0, 0));

  // use LIFO for each list
  if(BLK_SIZE(oldroot) > NOPREVFREE) PREV_FREE_ADDR(oldroot) = bp; // set the PREV_FREE_ADDR of the old root as the new root
  NEXT_FREE_ADDR(bp) = oldroot; // set the NEXT_FREE_ADDR of new root as oldroot
  if(BLK_SIZE(bp) > NOPREVFREE) PREV_FREE_ADDR(bp) = NULL; // set the PREV_FREE ADDR of the new root null

#ifdef DEBUG_LIST
printf(" -- segregate DONE for (%d=%d)\n", BLK_SIZE(bp), GET_SIZE(FTRP(bp)));
#endif

  return;
}

/*
 * unsegregate - takes the block out of the seglist.
 * if the block is root: set the next block as the new root
 * if the block is tail: set the prev block as the new tail
 * if the block isn't root nor tail: set the next block of the prev block as next block, the prev block of next block as prev block
 */
static void unsegregate(void *bp) {
  char **listblock = segblock(BLK_SIZE(bp));
  char *prevblock;
  char *nextblock;

#ifdef DEBUG_LIST
printf("| | | | UNsegregate BEGIN for %p(%d) - root:%d, tail:%d at %p ", bp, BLK_SIZE(bp), GET_ROOT(bp), GET_TAIL(bp), listblock);
#endif

  if (GET_ROOT(bp)) {

#ifdef DEBUG_LIST
printf(" -- bp is root of %p! ", listblock);
#endif

    if(GET_TAIL(bp)) { //bp is both root and tail

#ifdef DEBUG_LIST
printf(" -- bp is also tail! -- Unsegregate DONE\n");
#endif

      *listblock = NULL; // initialize the listblock root
      return;
    }
    //if bp is root
    nextblock = NEXT_FREE_ADDR(bp);
    *listblock = nextblock; // set root of listblock as the next free block
    if(BLK_SIZE(nextblock) > NOPREVFREE) PREV_FREE_ADDR(nextblock) = NULL; //set next block's prev addr null as it is the new root
    *HDRP(nextblock) = (*HDRP(nextblock) | 0x4); //set next block header as root
    *FTRP(nextblock) = (*FTRP(nextblock) | 0x4);
  } else if (GET_TAIL(bp)) { //bp is tail
#ifdef DEBUG_LIST
printf(" -- bp is tail!");
#endif
    prevblock = PREV_FREE_ADDR(bp); // get the prev free block
    if(!(BLK_SIZE(bp) > NOPREVFREE)) for(prevblock = *listblock; (NEXT_FREE_ADDR(prevblock)) != bp; prevblock = NEXT_FREE_ADDR(prevblock)); // get prev block even when it doesn't have prev block (due to small size)
    *HDRP(prevblock) = (*HDRP(prevblock) | 0x2); //set prevblock as tail
    *FTRP(prevblock) = (*FTRP(prevblock) | 0x2);
    NEXT_FREE_ADDR(prevblock) = NULL; //initialize nextaddr of the new tail
  } else { // bp isn't tail nor root
#ifdef DEBUG_LIST
printf(" -- bp isn't tail nor root\n");
#endif
    prevblock = PREV_FREE_ADDR(bp); // get the prev free block
    if(!(BLK_SIZE(bp) > NOPREVFREE)) for(prevblock = *listblock; (NEXT_FREE_ADDR(prevblock)) != bp; prevblock = NEXT_FREE_ADDR(prevblock)); // get prev block even when it doesn't have prev block (due to small size)
    nextblock = NEXT_FREE_ADDR(bp); // get the next free block
    NEXT_FREE_ADDR(prevblock) = nextblock; // set next addr of prev block as next block
    if(BLK_SIZE(nextblock) > NOPREVFREE) PREV_FREE_ADDR(nextblock) = prevblock; // prevblock can be defined if size of bp > NOPREVFREE
  }
#ifdef DEBUG_LIST
printf(" -- UNsegregation DONE\n");
#endif
  return;
}

/*
 * extend_heap - extends the heap by words
 */
static void *extend_heap(size_t words) {
#ifdef DEBUG
printf("| | extend_heap BEGIN -- size:%zu\n", words);
#endif

  char *bp;
  size_t size;

  /* Allocate an even number of words to maintain alignment */
  size = (words % 2) ? (words+1)*WSIZE : words*WSIZE;
  if((long)(bp = mem_sbrk(size)) == -1)
    return NULL;

  /* Initialize free block header/footre and the epilogue header */
  PUT(HDRP(bp), PACK(size, 0)); // Free block header
  PUT(FTRP(bp), PACK(size, 0)); // Free block footer
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); //New epilogue header
  lastblock = NEXT_BLKP(bp);

  /* Coalesce if the previous block was free */
  bp = coalesce(bp);

#ifdef DEBUG
printf("| | extend_heap DONE\n");
mm_check();
#endif

  return bp;
}

/*
 * coalesce - coalesces the block upon freeing an allocated block
 * both aren't empty - just return the block
 * prev is empty - return prev + block
 * next is empty - return block + next
 * both are empty - return prev + block + next
 * segregate and unsegregate accordingly
 */
static void *coalesce(void *bp) {
#ifdef DEBUG
printf("| | | coalesce BEGIN");
printf(" -- prev_blkp:%p(%d):%c, next_blkp: %p(%d):%c ", PREV_BLKP(bp), BLK_SIZE(PREV_BLKP(bp)), BLK_ALLOC_CHAR(PREV_BLKP(bp)), NEXT_BLKP(bp), BLK_SIZE(NEXT_BLKP(bp)), BLK_ALLOC_CHAR(NEXT_BLKP(bp)));
#endif

  size_t prev_alloc = BLK_ALLOC(PREV_BLKP(bp)); // checks if the prev block is allocated
  size_t next_alloc = BLK_ALLOC(NEXT_BLKP(bp)); // checks if the next block is allocated
  size_t size = BLK_SIZE(bp);

  if (prev_alloc && next_alloc) { // both not empty

#ifdef DEBUG
printf("(both not empty)");
printf(" -- coalesce DONE\n");
#endif

    segregate(bp); // segregate the newly free block
    return bp;
  }

  else if (prev_alloc && !next_alloc) { // next is empty

#ifdef DEBUG
printf("(next is empty)");
#endif

    unsegregate(NEXT_BLKP(bp)); // unsegregate the block which is going to be merged
    size += BLK_SIZE(NEXT_BLKP(bp));
    PUT(HDRP(bp), PACK(size, 0)); //| A |<-bp|   |
    PUT(FTRP(bp), PACK(size, 0)); //| A | bp | ->|  size is already implemented in HDRP so no need to NEXT_BLKP.
  }

  else if (!prev_alloc && next_alloc) { // prev is empty

#ifdef DEBUG
printf("(prev is empty)");
#endif

    unsegregate(PREV_BLKP(bp)); // unsegregate the block which is going to be merged
    size += BLK_SIZE(PREV_BLKP(bp));
    bp = PREV_BLKP(bp);
    PUT(HDRP(bp), PACK(size, 0));  //|<- | bp | A |
    PUT(FTRP(bp), PACK(size, 0));  //|   |bp->| A |
  }

  else { // both are empty

#ifdef DEBUG
printf("(both are empty)");
#endif

    unsegregate(NEXT_BLKP(bp)); // unsegregate the block which is going to be merged
    unsegregate(PREV_BLKP(bp));
    size += BLK_SIZE(PREV_BLKP(bp)) + BLK_SIZE(NEXT_BLKP(bp));
    bp = PREV_BLKP(bp);
    PUT(HDRP(bp), PACK(size, 0)); //|<- | bp|   |
    PUT(FTRP(bp), PACK(size, 0)); //|   | bp| ->|
  }

#ifdef DEBUG
printf(" -- coalesce DONE\n");
#endif

  segregate(bp); // segregate the newly free block
  return bp;
}

/*
 * find_from_list - looks for the best-fit block in the provided list.
 * It updates the closestblock whenever it comes upon a smaller block that is still larger than the required amount and returns it.
 * It returns null if the list is empty or if it doesn't have an adequate block
 */
static void *find_from_list(size_t asize, char **listblock) {
#ifdef DEBUG_LIST
printf("| | | | | find_from_list: %p(%p) ", listblock, *listblock);
#endif

  char *bp = 0;
  char *closestblock = NULL;

  if(*listblock == NULL) { // list is empty
#ifdef DEBUG_LIST
printf(" -- list is NULL -- end findlist\n");
#endif
    return NULL;
  }

  if(BLK_SIZE(*listblock) >= asize) // if the root is adequate
    closestblock = *listblock;

  for(bp = *listblock; GET_TAIL(bp) == 0 && NEXT_FREE_ADDR(bp) != NULL; bp = NEXT_FREE_ADDR(bp)) {
#ifdef DEBUG_LIST
if(BLK_ALLOC(bp))
  printf("==FATAL:ALLOCATED BLOCK(%p) IS IN THE SEGLIST==", bp);
#endif
    if(BLK_SIZE(bp) >= asize) { // it has to be larger than the required amount.
      if(closestblock == NULL || BLK_SIZE(closestblock) > BLK_SIZE(bp)) { // if closestblock is null or if there is a better block
        closestblock = bp;
#ifdef DEBUG_LIST
printf(" -- cb:%p(%d) --", bp, BLK_SIZE(bp));
#endif
      }
    }
  }
#ifdef DEBUG_LIST
printf(" -- found block! : %p\n", closestblock);
#endif
  return closestblock;
}

/*
 * find_smallest_from_list - looks for the smallest element on the provided list, and on the next list, and so on until the end.
 * It returns NULL if no such block has been found.
 */
static void *find_smallest_from_list(char **listblock) {
#ifdef DEBUG_LIST
printf("| | | | | find_smallest_from_list: %p(%p)", listblock, *listblock);
#endif
  char *bp = 0;
  char *smallestblock = NULL;

  if(listblock == LIST_PTR(list6, 6)) { // it reached the end of the blocks
#ifdef DEBUG_LIST
printf(" -- reached end of the blocks -- can't find\n");
#endif
    return NULL;
  }

  if(*listblock == NULL) { // if the list is empty looks for it on the next block
#ifdef DEBUG_LIST
printf(" -- searching next list..\n");
#endif
    return find_smallest_from_list(++listblock);
  }

  smallestblock = *listblock; // the root of the block if it's not empty
  for(bp = *listblock; GET_TAIL(bp) != 0 && NEXT_FREE_ADDR(bp) != NULL; bp = NEXT_FREE_ADDR(bp)) {
    if(BLK_SIZE(bp) < BLK_SIZE(smallestblock)) // if there is a smaller block, update
      smallestblock = bp;
  }

#ifdef DEBUG_LIST
printf(" -- found block! : %p\n", smallestblock);
#endif
  return smallestblock;
}

/*
 * find_fit - finds the fitting free block
 * it first searches the list where the asize falls in the range
 * it searches for the smallest one if no adequate block was found
 * it returns NULL if no adequate has been found still.
 */
static void *find_fit(size_t asize) {
#ifdef DEBUG_LIST
printf("| | | | find_fit BEGIN\n");
#endif

  char **listblock = (segblock(asize)); // get where to put this free block
  char *closestblock = NULL;

  //search blocks in the same range
  closestblock = find_from_list(asize, listblock);

  // search for the smallest of the larger blocks
  if(closestblock == NULL)
    closestblock = find_smallest_from_list(++listblock);

#ifdef DEBUG_LIST
if(closestblock != NULL)
  printf(" -- find_fit END:%p(%d) \n", closestblock, BLK_SIZE(closestblock));
else
  printf(" -- find_fit END:(nil) \n");
#endif

  return closestblock;
}

/*
 * place - places block with asize in bp accordingly
 * unsegregates the block, since it will no longer be free
 * sets the block as allocated
 * segregates the leftover block
 */
static void place(void *bp, size_t asize) {
#ifdef DEBUG
printf("| | place BEGIN at:%p(%d) size:%zu", bp, BLK_SIZE(bp), asize);
#endif
 
  size_t csize = BLK_SIZE(bp);

  // bp is no longer free
  unsegregate(bp);

  if((csize > asize + DSIZE)) { //Normal case
    // it is allocated now
    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));
    bp = NEXT_BLKP(bp);
    // leftover block
    PUT(HDRP(bp), PACK(csize-asize, 0));
    PUT(FTRP(bp), PACK(csize-asize, 0));
    segregate(bp); // segregate the leftover block
  } else {
    PUT(HDRP(bp), PACK(csize, 1));
    PUT(FTRP(bp), PACK(csize, 1));
  }

#ifdef DEBUG
printf(" -- place DONE\n");
mm_check();
#endif

  return;
}


