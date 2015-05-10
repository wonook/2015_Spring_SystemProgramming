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
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)    ((char *)(bp) - WSIZE)
#define FTRP(bp)    ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of its next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE((HDRP(bp))))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

static char *heap_listp = 0;


/* OWN FUNCTIONS */
#define DEBUG //for debugging

#define BLK_SIZE(bp) (GET_SIZE(HDRP(bp)))

int mm_init(range_t **ranges);
void* mm_malloc(size_t size);
void mm_free(void *ptr);
void* mm_realloc(void *ptr, size_t t);
void mm_exit(void);


int mm_check(void);
void *extend_heap(size_t words);
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
printf("    remove_range called\n");
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
#ifdef DEBUG
printf("  mm_init called\n");
#endif
  /* Create the initial empty heap */
  if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
    return -1;
  PUT(heap_listp, 0);
  PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));
  PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));
  PUT(heap_listp + (3*WSIZE), PACK(0, 1));
  heap_listp += (2*WSIZE); // Middle of prologue block

  /* Extend the empty heap with a free block of CHUNKSIZE bytes */
  if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
    return -1;

  /* DON't MODIFY THIS STAGE AND LEAVE IT AS IT WAS */
  gl_ranges = ranges;

#ifdef DEBUG
printf("  init done\n");
#endif

  return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void* mm_malloc(size_t size)
{
#ifdef DEBUG
printf("  mm_malloc called\n");
#endif
  size_t asize;
  size_t extendsize;
  char *bp;

  if (size == 0) return NULL;

  if (size <= DSIZE) asize = 2*DSIZE;
  else asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

  if((bp = find_fit(asize)) != NULL) {
    place(bp, asize);
    return bp;
  }

  extendsize = MAX(asize, CHUNKSIZE);
  if((bp = extend_heap(extendsize/WSIZE)) == NULL) return NULL;
  place(bp, asize);
  return bp;
  /*int newsize = ALIGN(size + SIZE_T_SIZE);*/
  /*void *p = mem_sbrk(newsize);*/
 /*if (p == (void *)-1)*/
    /*return NULL;*/
  /*else {*/
    /**(size_t *)p = size;*/
    /*return (void *)((char *)p + SIZE_T_SIZE);*/
  /*}*/
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
  /* YOUR IMPLEMENTATION */
#ifdef DEBUG
printf("  mm_free called\n");
#endif

#ifdef DEBUG
  printf("      reaches here?");
#endif
  size_t size = GET_SIZE(HDRP(ptr));

  PUT(HDRP(ptr), PACK(size, 0));
  PUT(FTRP(ptr), PACK(size, 0));
  coalesce(ptr);

  /* DON't MODIFY THIS STAGE AND LEAVE IT AS IT WAS */
  if (gl_ranges)
    remove_range(gl_ranges, ptr);
}

/*
 * mm_realloc - empty implementation; YOU DO NOT NEED TO IMPLEMENT THIS
 */
void* mm_realloc(void *ptr, size_t t)
{
#ifdef DEBUG
printf("  !!mm_realloc called\n");
#endif

  return NULL;
}

/*
 * mm_exit - finalize the malloc package.
 */
void mm_exit(void)
{
#ifdef DEBUG
printf("  mm_exit called\n");
#endif

}

/* EXTRA FUNCTIONS */
/*
 * mm_check - checks heap consistency
 */
int mm_check(void) {
#ifdef DEBUG
printf("   mm_check called\n");
#endif

  return 0;
}

/*
 * extend_heap - extends the heap
 */
void *extend_heap(size_t words) {
#ifdef DEBUG
printf("    extend_heap called\n");
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

  /* Coalesce if the previous block was free */
  return coalesce(bp);
}

/*
 * coalesce - coalesces the block upon freeing
 */
static void *coalesce(void *bp) {
#ifdef DEBUG
printf("    coalesce called\n");
#endif
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));

  if (prev_alloc && next_alloc) { // both not empty
    return bp;
  }

  else if (prev_alloc && !next_alloc) { // next is empty
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
  }

  else if (prev_alloc && next_alloc) { // prev is empty
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }

  else { // both are empty
    size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }

  return bp;
}

/*
 * find_fit - 
 */
static void *find_fit(size_t asize) {
  /* First fit search */
  void *bp;

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
  size_t csize = GET_SIZE(HDRP(bp));

  if((csize - asize) <= (2*DSIZE)) {
    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));
    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(csize-asize, 0));
    PUT(FTRP(bp), PACK(csize-asize, 0));
  } else {
    PUT(HDRP(bp), PACK(csize, 1));
    PUT(FTRP(bp), PACK(csize, 1));
  }

}


