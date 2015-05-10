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


/* FUNCTIONS FROM THE BOOK */
#define WSIZE 4 // Single wods zie in bytes
#define DSIZE 8 // Double word size in bytes
#define CHUNKSIZE (1<<12) //extend the heap by this amount

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


/* OWN FUNCTIONS */
#define DEBUG //for debugging
#define DEBUG_STATUS

#define BLK_SIZE(bp) (GET_SIZE(HDRP(bp)))
#define BLK_ALLOC(bp) (GET_ALLOC(HDRP(bp)))
#define BLK_ALLOC_CHAR(bp) ((BLK_ALLOC(bp)) ? 'A' : 'F')

int mm_init(range_t **ranges);
void* mm_malloc(size_t size);
void mm_free(void *ptr);
void* mm_realloc(void *ptr, size_t t);
void mm_exit(void);


int mm_check(void);
void printheap(void);

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);



/* 
 * remove_range - manipulate range lists
 * DON'T MODIFY THIS FUNCTION AND LEAVE IT AS IT WAS
 */
static void remove_range(range_t **ranges, char *lo)
{
#ifdef DEBUG
printf("| | remove_range BEGINDONE\n");
#endif
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
printf("\n| mm_init BEGIN\n");
#endif
  /* Create the initial empty heap */
  if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
    return -1;
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

#ifdef DEBUG
mm_check();
#endif
#ifdef DEBUG_STATUS
printf("| init DONE\n");
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
printf("| mm_malloc BEGIN\n");
#endif
#ifdef DEBUG
mm_check();
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
printf("| | malloc - size: %zu, adjusted size: %zu \n", size, asize);
#endif

  if((bp = find_fit(asize)) != NULL) {
#ifdef DEBUG
printf("| | found fit block\n");
#endif
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
printf("| mm_free BEGIN\n");
#endif
#ifdef DEBUG
mm_check();
printf("| | free => ptr: %p\n", ptr);
#endif
  size_t size = BLK_SIZE(ptr);

  PUT(HDRP(ptr), PACK(size, 0));
  PUT(FTRP(ptr), PACK(size, 0));
  coalesce(ptr);

  /* DON't MODIFY THIS STAGE AND LEAVE IT AS IT WAS */
  if (gl_ranges)
    remove_range(gl_ranges, ptr);
#ifdef DEBUG
mm_check();
#endif
#ifdef DEBUG_STATUS
printf("| mm_free DONE\n");
#endif
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
printf("| mm_exit BEGINDONE\n");
#endif

}

/* ===============================EXTRA FUNCTIONS========================= */
/* ===============================  FOR TESTING  ========================= */
/*
 * mm_check - checks heap consistency
 */
int mm_check(void) {
  printheap();

  return 0;
}

/*
 * printheap - prints out the heap
 */
void printheap(void) {
  printf("\tCURRENT HEAP: ");
  void *bp = 0;
  size_t size;
  char r;

  for(bp = heap_listp; BLK_SIZE(bp) > 0; bp = NEXT_BLKP(bp)) {
    size = (BLK_SIZE(bp));
    printf("|%7zu%2c |", size, BLK_ALLOC_CHAR(bp));
  }
  printf("\n\t\t      ");
  for(bp = heap_listp; BLK_SIZE(bp) > 0; bp = NEXT_BLKP(bp)) {
    printf("|%p|", bp);
  }
 
  printf("\n");
  return;
}

/* ===============================EXTRA FUNCTIONS========================= */
/*
 * extend_heap - extends the heap
 */
static void *extend_heap(size_t words) {
#ifdef DEBUG
printf("| | extend_heap BEGIN\n");
mm_check();
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
mm_check();
printf("| | extend_heap DONE\n");
#endif
  /* Coalesce if the previous block was free */
  return bp;
}

/*
 * coalesce - coalesces the block upon freeing
 */
static void *coalesce(void *bp) {
#ifdef DEBUG
printf("| | coalesce BEGIN\n");
printf("| | | prev_blkp:%p:%c, next_blkp: %p:%c\n", PREV_BLKP(bp), BLK_ALLOC_CHAR(PREV_BLKP(bp)), NEXT_BLKP(bp), BLK_ALLOC_CHAR(NEXT_BLKP(bp)));
#endif
  size_t prev_alloc = BLK_ALLOC(PREV_BLKP(bp));
  size_t next_alloc = BLK_ALLOC(NEXT_BLKP(bp));
  size_t size = BLK_SIZE(bp);

  if (prev_alloc && next_alloc) { // both not empty
#ifdef DEBUG
printf("| | | both not empty\n");
printf("| | coalesce DONE\n");
#endif
    return bp;
  }

  else if (prev_alloc && !next_alloc) { // next is empty
#ifdef DEBUG
printf("| | | next is empty\n");
#endif
    size += BLK_SIZE(NEXT_BLKP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
  }

  else if (!prev_alloc && next_alloc) { // prev is empty
#ifdef DEBUG
printf("| | | prev is empty\n");
#endif
    size += BLK_SIZE(PREV_BLKP(bp));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }

  else { // both are empty
#ifdef DEBUG
printf("| | | both are empty\n");
#endif
    size += BLK_SIZE(PREV_BLKP(bp)) + BLK_SIZE(NEXT_BLKP(bp));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }

#ifdef DEBUG
printf("| | coalesce DONE\n");
#endif
  return bp;
}

/*
 * find_fit - 
 */
static void *find_fit(size_t asize) {
#ifdef DEBUG
printf("| | find_fit BEGINDONE\n");
#endif
 
  /* First fit search */
  void *bp = 0;

  for(bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
    if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
      return bp;
    }
  }
  return NULL;
}

/*
 * place - 
 */
static void place(void *bp, size_t asize) {
#ifdef DEBUG
printf("| | place BEGIN\n");
#endif
 
  size_t csize = BLK_SIZE(bp);

  if((csize - asize - (2*DSIZE)) >= 0) {
    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));
    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(csize-asize, 0));
    PUT(FTRP(bp), PACK(csize-asize, 0));
  } else {
    PUT(HDRP(bp), PACK(csize, 1));
    PUT(FTRP(bp), PACK(csize, 1));
  }
#ifdef DEBUG
mm_check();
printf("| | place DONE\n");
#endif
}


