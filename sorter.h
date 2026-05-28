#ifndef SORTER_H
#define SORTER_H

#include <stdbool.h>

typedef enum {
    ALGO_BUBBLE,
    ALGO_SELECTION,
    ALGO_INSERTION,
    ALGO_QUICK,
    ALGO_MERGE,
    ALGO_HEAP,
    ALGO_SHELL,
    ALGO_COCKTAIL,
    ALGO_GNOME,
    ALGO_RADIX,
    ALGO_COUNT
} Algorithm;

extern const char *ALGO_NAMES[ALGO_COUNT];

typedef struct {
    int *arr;
    int n;
    int max_val;
    Algorithm algo;
    bool sorting;
    bool sorted;
    int comparisons;
    int swaps;
    int delay_ms;
    int current_idx;
    int current_idx2;
    int sorted_until;
    bool quit;
} SortState;

void init_state(SortState *s, int n);
void free_state(SortState *s);
void shuffle_array(SortState *s);
void run_sort(SortState *s);

void on_sort_step(SortState *s);

#endif
