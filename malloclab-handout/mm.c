/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
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
#define WSIZE 4 // Single wods zie in bytes
#define DSIZE 8 // Double word size in bytes
#define CHUNKSIZE (1<<10) //extend the heap by this amount

#define MAX(x, y)   ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word (for header/footer) */
#define PACK(size, alloc)   ((size)|(alloc))

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

/* OWN MACROS */
#define DEBUG //for debugging
#define DEBUG_STATUS

#define BLK_SIZE(bp) (GET_SIZE(HDRP(bp)))
#define BLK_ALLOC(bp) (GET_ALLOC(HDRP(bp)))
#define BLK_ALLOC_CHAR(bp) ((BLK_ALLOC(bp)) ? 'A' : 'F')

#define SIZE1 16
#define SIZE2 64
#define SIZE3 256
#define SIZE4 1024
#define SIZE5 4096
#define SIZE6 16384
#define SIZE7 65536

#define LIST_PTR(lp, num) ((char *)(lp) + (num * (WSIZE))) //use is as LIST_PTR(list1, 1) to get list ptr to blocks of 16
#define ROOT_ADDR(lp) (lp)
#define ROOT_BLK(lp) (*(char *)(lp))
#define LIST_ALLOC(lp) (((lp) == NULL) ? 0 : 1) // sees if this seglist block is free (if there are any free blocks or not) -- to assign one, use PUT(bp, PACK(address, 1))
#define LIST_ALLOC_CHAR(lp) (((lp) == NULL) ? 'F' : 'A')

#define GET_ROOT(bp) (GET(HDRP(bp)) & 0x4) // sees if the free block is seglist ROOT
#define GET_TAIL(bp) (GET(HDRP(bp)) & 0x2) // sees if the free block is seglist TAIL

#define FREEPACK(size, root, tail, alloc) ((size)|(root)|(tail)|(alloc)) // packs information for the header/footer of the free block

#define NEXT_FREE_ADDR(bp) (*(char **)(bp)) // gets address of next free block on the list
#define PREV_FREE_ADDR(bp) (*((char **)(bp) + WSIZE)) // gets address of previous free block on the list
#define SET_NEXT_FREE_ADDR(bp, addr) ((NEXT_FREE_ADDR(bp)) = addr) // assign address of next free block
#define SET_PREV_FREE_ADDR(bp, addr) ((PREV_FREE_ADDR(bp)) = addr)


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
 * mm_init - initialize the malloc package.
 */
int mm_init(range_t **ranges)
{
  /* YOUR IMPLEMENTATION */
#ifdef DEBUG_STATUS
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
    *listblock = NULL;//initialize seglist : set ROOT_ADDR as 0
#ifdef DEBUG
/*printf(" | initialized %p to %p | ", listblock, *listblock);*/
#endif
  }

  /* empty heap */
  PUT(heap_listp, 0);
  PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));
  PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));
  PUT(heap_listp + (3*WSIZE), PACK(0, 1));
  heap_listp += (2*WSIZE); // Middle of prologue block

#ifdef DEBUG
mm_check();
#endif

  /* Extend the empty heap with a free block of CHUNKSIZE bytes */
  if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
    return -1;

  /* DON't MODIFY THIS STAGE AND LEAVE IT AS IT WAS */
  gl_ranges = ranges;

#ifdef DEBUG_STATUS
printf("| init DONE\n");
#endif
#ifdef DEBUG
mm_check();
#endif

  return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void* mm_malloc(size_t size)
{
#ifdef DEBUG_STATUS
printf("| mm_malloc BEGIN ");
#endif

  size_t asize;
  size_t extendsize;
  char *bp;

  if (heap_listp == 0) 
    mm_init(NULL); // WARN: not sure if used correctly

  if (size == 0) return NULL;

  if (size <= DSIZE) asize = 2*DSIZE;
  else asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

#ifdef DEBUG
printf(" -- size: %zu, adjusted size: %zu \n", size, asize);
#endif

  if((bp = find_fit(asize)) != NULL) {
    place(bp, asize);

#ifdef DEBUG
printf("| mm_malloc DONE\n");
#endif

    return bp;
  }

  extendsize = MAX(asize, CHUNKSIZE);
  if((bp = extend_heap(extendsize/WSIZE)) == NULL) return NULL;
  place(bp, asize);

#ifdef DEBUG_STATUS
printf("| mm_malloc DONE\n");
#endif

  return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
  /* YOUR IMPLEMENTATION */
#ifdef DEBUG_STATUS
printf("| mm_free BEGIN at:%p\n", ptr);
#endif
  size_t size = BLK_SIZE(ptr);

  PUT(HDRP(ptr), PACK(size, 0));
  PUT(FTRP(ptr), PACK(size, 0));
  coalesce(ptr);

  /* DON't MODIFY THIS STAGE AND LEAVE IT AS IT WAS */
  if (gl_ranges)
    remove_range(gl_ranges, ptr);

#ifdef DEBUG_STATUS
printf("| mm_free DONE\n");
#endif
#ifdef DEBUG
mm_check();
#endif

  return;
}

/*
 * mm_realloc - empty implementation; YOU DO NOT NEED TO IMPLEMENT THIS
 */
void* mm_realloc(void *ptr, size_t t)
{
#ifdef DEBUG_STATUS
printf("| !!mm_realloc BEGINDONE\n");
#endif

  return NULL;
}

/*
 * mm_exit - finalize the malloc package.
 */
void mm_exit(void)
{
#ifdef DEBUG_STATUS
printf("| mm_exit BEGIN\n");
#endif
  void *bp = heap_listp;

  for(bp = NEXT_BLKP(bp); BLK_SIZE(bp) > 0; bp = NEXT_BLKP(bp)) {
    if(BLK_ALLOC(bp)) mm_free(bp);
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
 * mm_check - checks heap consistency
 */
int mm_check(void) {
  printheap();
  printlist();
  return 0;
}

/*
 * printheap - prints out the heap
 */
void printheap(void) {
  printf("\tCURRENT HEAP: ");
  void *bp = 0;
  size_t size;

  for(bp = heap_listp; BLK_SIZE(bp) > 0; bp = NEXT_BLKP(bp)) {
    size = (BLK_SIZE(bp));
    printf("| %p:%zu%2c |", bp, size, BLK_ALLOC_CHAR(bp));
  }
  size = (BLK_SIZE(bp));
  printf("| %p:%zu%2c |\n", bp, size, BLK_ALLOC_CHAR(bp));
  return;
}

void printlist(void) {
  printf("\tCURRENT LIST: ");
  char **listblock;
  int i;

  listblock = list1;
  for(i=0; listblock < list2; listblock = LIST_PTR(list1, ++i)) {
    printf("| %p:%5d->%p |", listblock, SIZE1 + (i * (SIZE1 / 2)), *listblock);
  } printf("\n\t              ");

  for(i=0; listblock < list3; listblock = LIST_PTR(list2, ++i)) {
    printf("| %p:%5d->%p |", listblock, SIZE2 + (i * (SIZE2 / 2)), *listblock);
  } printf("\n\t              ");

  for(i=0; listblock < list4; listblock = LIST_PTR(list3, ++i)) {
    printf("| %p:%5d->%p |", listblock, SIZE3 + (i * (SIZE3 / 2)), *listblock);
  } printf("\n\t              ");

  for(i=0; listblock < list5; listblock = LIST_PTR(list4, ++i)) {
    printf("| %p:%5d->%p |", listblock, SIZE4 + (i * (SIZE4 / 2)), *listblock);
  } printf("\n\t              ");

  for(i=0; listblock < list6; listblock = LIST_PTR(list5, ++i)) {
    printf("| %p:%5d->%p |", listblock, SIZE5 + (i * (SIZE5 / 2)), *listblock);
  } printf("\n\t              ");

  for(i=0; listblock < list6 + 6 * WSIZE; listblock = LIST_PTR(list6, ++i)) {
    printf("| %p:%5d->%p |", listblock, SIZE6 + (i * (SIZE6 / 2)), *listblock);
  } printf("\n");
}

/* ===============================EXTRA FUNCTIONS========================= */

/*
 * adjustsize - returns adjusted size
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
 * segregate-
 */
static void segregate(void *bp) {
  size_t size = BLK_SIZE(bp);
  char **listblock = (segblock(size)); // get where to put this free block

#ifdef DEBUG
printf("| | | | segregate BEGIN for %p(%d) to %p:%c", bp, BLK_SIZE(bp), listblock, LIST_ALLOC_CHAR(*listblock));
if(BLK_ALLOC(bp)) printf("  BLOCK IS ALLOCATED BUT TRYING TO SEGREGATE!!");
#endif

  if(!(LIST_ALLOC(*listblock))) { //if seglist is empty
    *listblock = bp;
    PREV_FREE_ADDR(bp) = listblock;
    NEXT_FREE_ADDR(bp) = NULL;
    PUT(HDRP(bp), FREEPACK(size, 4, 2, 0));
    PUT(FTRP(bp), FREEPACK(size, 4, 2, 0));
#ifdef DEBUG
printf(" -- seglist is empty! segregate DONE: %p, root: %d, tail: %d\n", bp, GET_ROOT(bp), GET_TAIL(bp));
#endif
    return;
  }

  char *oldroot;
  oldroot = *listblock; // get the old root
  *listblock = bp; //push the new root onto the seglist
  (*HDRP(oldroot) = (*HDRP(oldroot) & ~0x4)); //not root anymore
  (*FTRP(oldroot) = (*FTRP(oldroot) & ~0x4));
  (*HDRP(bp) = (*HDRP(bp) | 0x4)); //bp as new root
  (*FTRP(bp) = (*FTRP(bp) | 0x4));

  PREV_FREE_ADDR(oldroot) = bp;
  NEXT_FREE_ADDR(bp) = oldroot;
  PREV_FREE_ADDR(bp) = listblock;
#ifdef DEBUG
printf(" -- segregate DONE\n");
#endif
  return;
}

/*
 * unsegregate-
 */
static void unsegregate(void *bp) {
#ifdef DEBUG
printf("| | | | UNsegregate BEGIN for %p(%d) - root:%d, tail:%d ", bp, BLK_SIZE(bp), GET_ROOT(bp), GET_TAIL(bp));
#endif

  char **listblock;
  char *prevblock;
  char *nextblock;

  if (GET_ROOT(bp)) { //bp is root
#ifdef DEBUG
printf(" -- bp is root!");
#endif
    listblock = PREV_FREE_ADDR(bp);
    if(GET_TAIL(bp)) { //both root and tail
#ifdef DEBUG
printf(" -- bp is also tail!");
#endif
      *listblock = NULL; // initialize the listblock
      return;
    }
    nextblock = NEXT_FREE_ADDR(bp);
    *listblock = nextblock; // set root of listblock to next free block
    PREV_FREE_ADDR(nextblock) = listblock; //set next block's prev addr as listblock
    *HDRP(nextblock) = (*HDRP(nextblock) | 0x4); //set next block as root
    *FTRP(nextblock) = (*FTRP(nextblock) | 0x4);
  } else if (GET_TAIL(bp)) { //bp is tail
#ifdef DEBUG
printf(" -- bp is tail!");
#endif
    prevblock = PREV_FREE_ADDR(bp);
    *HDRP(prevblock) = (*HDRP(prevblock) | 0x2); //set prevblock as tail
    *FTRP(prevblock) = (*FTRP(prevblock) | 0x2);
    NEXT_FREE_ADDR(prevblock) = 0; //initialize nextaddr of new tail
  } else {
#ifdef DEBUG
printf(" -- bp isn't tail nor root");
#endif
    prevblock = PREV_FREE_ADDR(bp);
    nextblock = NEXT_FREE_ADDR(bp);
    NEXT_FREE_ADDR(prevblock) = nextblock;
    PREV_FREE_ADDR(nextblock) = prevblock;
  }
#ifdef DEBUG
printf(" -- UNsegregation DONE\n");
#endif
  return;
}

/*
 * extend_heap - extends the heap
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

  bp = coalesce(bp);

#ifdef DEBUG
printf("| | extend_heap DONE\n");
mm_check();
#endif
  /* Coalesce if the previous block was free */
  return bp;
}

/*
 * coalesce - coalesces the block upon freeing
 */
static void *coalesce(void *bp) {
#ifdef DEBUG
printf("| | | coalesce BEGIN");
printf(" -- prev_blkp:%p:%c, next_blkp: %p:%c ", PREV_BLKP(bp), BLK_ALLOC_CHAR(PREV_BLKP(bp)), NEXT_BLKP(bp), BLK_ALLOC_CHAR(NEXT_BLKP(bp)));
#endif

  size_t prev_alloc = BLK_ALLOC(PREV_BLKP(bp));
  size_t next_alloc = BLK_ALLOC(NEXT_BLKP(bp));
  size_t size = BLK_SIZE(bp);

  if (prev_alloc && next_alloc) { // both not empty

#ifdef DEBUG
printf("(both not empty)");
printf(" -- coalesce DONE\n");
#endif

    segregate(bp);
    return bp;
  }

  else if (prev_alloc && !next_alloc) { // next is empty

#ifdef DEBUG
printf("(next is empty)");
#endif

    unsegregate(NEXT_BLKP(bp));
    size += BLK_SIZE(NEXT_BLKP(bp));
    PUT(HDRP(bp), PACK(size, 0)); //| A |<-bp|   |
    PUT(FTRP(bp), PACK(size, 0)); //| A | bp | ->|  size is already implemented in HDRP so no need to NEXT_BLKP.
  }

  else if (!prev_alloc && next_alloc) { // prev is empty

#ifdef DEBUG
printf("(prev is empty)");
#endif

    unsegregate(PREV_BLKP(bp));
    size += BLK_SIZE(PREV_BLKP(bp));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));  //|<- | bp | A |
    PUT(FTRP(bp), PACK(size, 0));             //|<- |bp->| A |
    bp = PREV_BLKP(bp);
  }

  else { // both are empty

#ifdef DEBUG
printf("(both are empty)");
#endif

    unsegregate(NEXT_BLKP(bp));
    unsegregate(PREV_BLKP(bp));
    size += BLK_SIZE(PREV_BLKP(bp)) + BLK_SIZE(NEXT_BLKP(bp));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); //|<- | bp|   |
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0)); //|   | bp| ->|
    bp = PREV_BLKP(bp);
  }

#ifdef DEBUG
printf(" -- coalesce DONE\n");
#endif

  segregate(bp);
  return bp;
}

/**/
static void *find_from_list(size_t asize, char **listblock) {
#ifdef DEBUG
printf("| | | | | find_from_list: %p(%p)\n", listblock, *listblock);
#endif

  char *bp = 0;
  char *closestblock = NULL;

  if(*listblock == NULL) {
#ifdef DEBUG
printf(" -- list is NULL -- end findlist\n");
#endif
    return NULL;
  }

  closestblock = *listblock;
  for(bp = *listblock; GET_TAIL(bp) != 0 && NEXT_FREE_ADDR(bp) != NULL; bp = NEXT_FREE_ADDR(bp)) {
    if(BLK_SIZE(bp) > asize) {
      if(BLK_SIZE(closestblock) > BLK_SIZE(bp)) {
        closestblock = bp;
      }
    }
  }
#ifdef DEBUG
printf(" -- found block! : %p\n", closestblock);
#endif
  return closestblock;
}

/**/
static void *find_smallest_from_list(char **listblock) {
#ifdef DEBUG
printf("| | | | | find_smallest_from_list: %p(%p)\n", listblock, *listblock);
#endif
  char *bp = 0;
  char *smallestblock = NULL;

  if(listblock == LIST_PTR(list6, 6)) {
#ifdef DEBUG
printf(" -- reached end of the blocks -- can't find\n");
#endif
    return NULL;
  }

  if(*listblock == NULL) {
#ifdef DEBUG
printf(" -- searching next list..\n");
#endif
    return find_smallest_from_list(++listblock);
  }

  smallestblock = *listblock;
  for(bp = *listblock; GET_TAIL(bp) != 0 && NEXT_FREE_ADDR(bp) != NULL; bp = NEXT_FREE_ADDR(bp)) {
    if(BLK_SIZE(bp) < BLK_SIZE(smallestblock))
      smallestblock = bp;
  }

#ifdef DEBUG
printf(" -- found block! : %p\n", smallestblock);
#endif
  return smallestblock;
}

/*
 * find_fit - finds the fitting free block
 */
static void *find_fit(size_t asize) {
  // /* First fit search */
  // void *bp = 0;

  // for(bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
  //   if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
  //     return bp;
  //   }
  // }
  // return NULL;
#ifdef DEBUG
printf("| | | | find_fit BEGIN\n");
#endif

  char **listblock = (segblock(asize)); // get where to put this free block
  char *closestblock = NULL;

  //search smaller blocks
  closestblock = find_from_list(asize, listblock);

  if(closestblock == NULL)
    closestblock = find_smallest_from_list(++listblock);

#ifdef DEBUG
if(closestblock != NULL)
  printf(" -- find_fit END:%p(%d) \n", closestblock, BLK_SIZE(closestblock));
else
  printf(" -- find_fit END:(nil) \n");
#endif

  return closestblock;
}

/*
 * place - places asize block in bp accordingly
 */
static void place(void *bp, size_t asize) {
#ifdef DEBUG
printf("| | place BEGIN at:%p size:%zu ", bp, asize);
#endif
 
  size_t csize = BLK_SIZE(bp);

  if((csize > asize + (2*DSIZE))) { //Normal case
    PUT(HDRP(bp), FREEPACK(asize, GET_ROOT(bp), GET_TAIL(bp), 1));
    PUT(FTRP(bp), FREEPACK(asize, GET_ROOT(bp), GET_TAIL(bp), 1));
    unsegregate(bp);
    bp = NEXT_BLKP(bp);
    if(csize==asize) return;
    PUT(HDRP(bp), PACK(csize-asize, 0));
    PUT(FTRP(bp), PACK(csize-asize, 0));
    segregate(bp);
  } else {
    PUT(HDRP(bp), FREEPACK(csize, GET_ROOT(bp), GET_TAIL(bp), 1));
    PUT(FTRP(bp), FREEPACK(csize, GET_ROOT(bp), GET_TAIL(bp), 1));
    unsegregate(bp);
  }

#ifdef DEBUG
printf(" -- place DONE\n");
mm_check();
#endif

  return;
}


