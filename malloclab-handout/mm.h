#include <stdio.h>

extern int mm_init (void);
extern void *mm_malloc (size_t size);
extern void mm_free (void *ptr);

/*
 * Students work in teams of one or two.  Teams enter their team name,
 * personal names and student IDs in a struct of this
 * type in their mm.c file.
 */
typedef struct {
    char *teamname; /* ID1+ID2 or ID1 */
    char *name1;    /* full name of first member */
    char *id1;      /* student ID of first member */
    char *name2;    /* full name of second member (if any) */
    char *id2;      /* student ID of second member */
} team_t;

extern team_t team;

// this is here for completeness, you don't need to implement this!
extern void *mm_realloc(void *ptr, size_t size);
