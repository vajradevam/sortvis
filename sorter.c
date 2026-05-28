#include "sorter.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

const char *ALGO_NAMES[ALGO_COUNT] = {
    "Bubble Sort",
    "Selection Sort",
    "Insertion Sort",
    "Quick Sort",
    "Merge Sort",
    "Heap Sort",
    "Shell Sort",
    "Cocktail Sort",
    "Gnome Sort",
    "Radix Sort",
};

static void swap(int *a, int *b) {
    int t = *a;
    *a = *b;
    *b = t;
}

void init_state(SortState *s, int n) {
    s->n = n;
    s->max_val = n;
    s->arr = (int *)malloc(n * sizeof(int));
    s->algo = ALGO_BUBBLE;
    s->sorting = false;
    s->sorted = false;
    s->comparisons = 0;
    s->swaps = 0;
    s->delay_ms = 20;
    s->current_idx = -1;
    s->current_idx2 = -1;
    s->sorted_until = 0;
    s->quit = false;
    shuffle_array(s);
}

void free_state(SortState *s) {
    free(s->arr);
    s->arr = NULL;
}

void shuffle_array(SortState *s) {
    for (int i = 0; i < s->n; i++)
        s->arr[i] = i + 1;
    for (int i = s->n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int t = s->arr[i];
        s->arr[i] = s->arr[j];
        s->arr[j] = t;
    }
    s->sorted = false;
    s->sorting = false;
    s->sorted_until = 0;
    s->comparisons = 0;
    s->swaps = 0;
    s->current_idx = -1;
    s->current_idx2 = -1;
}

// ----------------------------------------------------------------

static void bubble_sort(SortState *s) {
    for (int i = 0; i < s->n - 1 && !s->quit; i++) {
        for (int j = 0; j < s->n - i - 1 && !s->quit; j++) {
            s->current_idx = j;
            s->current_idx2 = j + 1;
            s->comparisons++;
            on_sort_step(s);
            if (s->arr[j] > s->arr[j + 1]) {
                swap(&s->arr[j], &s->arr[j + 1]);
                s->swaps++;
                on_sort_step(s);
            }
        }
        s->sorted_until = s->n - i - 1;
    }
    s->sorted_until = s->n;
}

static void selection_sort(SortState *s) {
    for (int i = 0; i < s->n - 1 && !s->quit; i++) {
        int min_idx = i;
        for (int j = i + 1; j < s->n && !s->quit; j++) {
            s->current_idx = j;
            s->current_idx2 = min_idx;
            s->comparisons++;
            on_sort_step(s);
            if (s->arr[j] < s->arr[min_idx])
                min_idx = j;
        }
        if (min_idx != i) {
            swap(&s->arr[i], &s->arr[min_idx]);
            s->swaps++;
            on_sort_step(s);
        }
        s->sorted_until = i + 1;
    }
    s->sorted_until = s->n;
}

static void insertion_sort(SortState *s) {
    for (int i = 1; i < s->n && !s->quit; i++) {
        int key = s->arr[i];
        int j = i - 1;
        while (j >= 0 && s->arr[j] > key && !s->quit) {
            s->current_idx = j;
            s->current_idx2 = j + 1;
            s->comparisons++;
            s->arr[j + 1] = s->arr[j];
            s->swaps++;
            on_sort_step(s);
            j--;
        }
        s->arr[j + 1] = key;
        if (j + 1 != i) {
            s->current_idx = j + 1;
            s->current_idx2 = i;
            on_sort_step(s);
        }
        s->sorted_until = i + 1;
    }
    s->sorted_until = s->n;
}

static void quick_sort(SortState *s) {
    int *stack = (int *)malloc(s->n * 2 * sizeof(int));
    int top = -1;
    stack[++top] = 0;
    stack[++top] = s->n - 1;

    while (top >= 0 && !s->quit) {
        int high = stack[top--];
        int low = stack[top--];

        int pivot = s->arr[high];
        int i = low - 1;

        for (int j = low; j < high && !s->quit; j++) {
            s->current_idx = j;
            s->current_idx2 = i + 1;
            s->comparisons++;
            on_sort_step(s);
            if (s->arr[j] < pivot) {
                i++;
                swap(&s->arr[i], &s->arr[j]);
                s->swaps++;
                on_sort_step(s);
            }
        }
        swap(&s->arr[i + 1], &s->arr[high]);
        s->swaps++;
        s->current_idx = i + 1;
        on_sort_step(s);

        int pi = i + 1;

        if (pi + 1 < high) {
            stack[++top] = pi + 1;
            stack[++top] = high;
        }
        if (pi - 1 > low) {
            stack[++top] = low;
            stack[++top] = pi - 1;
        }
    }
    free(stack);
    s->sorted_until = s->n;
}

static void merge_sort(SortState *s) {
    int *aux = (int *)malloc(s->n * sizeof(int));

    for (int width = 1; width < s->n && !s->quit; width *= 2) {
        for (int left = 0; left < s->n && !s->quit; left += 2 * width) {
            int mid = left + width - 1;
            int right = left + 2 * width - 1;
            if (mid >= s->n - 1) continue;
            if (right >= s->n) right = s->n - 1;

            int i = left, j = mid + 1, k = left;
            while (i <= mid && j <= right && !s->quit) {
                s->current_idx = i;
                s->current_idx2 = j;
                s->comparisons++;
                on_sort_step(s);
                if (s->arr[i] <= s->arr[j])
                    aux[k++] = s->arr[i++];
                else
                    aux[k++] = s->arr[j++];
            }
            while (i <= mid && !s->quit) {
                s->current_idx = i;
                on_sort_step(s);
                aux[k++] = s->arr[i++];
            }
            while (j <= right && !s->quit) {
                s->current_idx = j;
                on_sort_step(s);
                aux[k++] = s->arr[j++];
            }
            for (i = left; i <= right && !s->quit; i++) {
                s->arr[i] = aux[i];
                s->current_idx = i;
                on_sort_step(s);
            }
        }
    }
    free(aux);
    s->sorted_until = s->n;
}

static void heapify(SortState *s, int n, int i) {
    int largest = i;
    int left = 2 * i + 1;
    int right = 2 * i + 2;

    if (left < n) {
        s->comparisons++;
        s->current_idx = left;
        s->current_idx2 = largest;
        on_sort_step(s);
        if (!s->quit && s->arr[left] > s->arr[largest])
            largest = left;
    }
    if (!s->quit && right < n) {
        s->comparisons++;
        s->current_idx = right;
        s->current_idx2 = largest;
        on_sort_step(s);
        if (!s->quit && s->arr[right] > s->arr[largest])
            largest = right;
    }
    if (!s->quit && largest != i) {
        swap(&s->arr[i], &s->arr[largest]);
        s->swaps++;
        on_sort_step(s);
        heapify(s, n, largest);
    }
}

static void heap_sort(SortState *s) {
    for (int i = s->n / 2 - 1; i >= 0 && !s->quit; i--)
        heapify(s, s->n, i);
    for (int i = s->n - 1; i > 0 && !s->quit; i--) {
        swap(&s->arr[0], &s->arr[i]);
        s->swaps++;
        s->sorted_until = i;
        on_sort_step(s);
        heapify(s, i, 0);
    }
    s->sorted_until = s->n;
}

static void shell_sort(SortState *s) {
    for (int gap = s->n / 2; gap > 0 && !s->quit; gap /= 2) {
        for (int i = gap; i < s->n && !s->quit; i++) {
            int temp = s->arr[i];
            int j;
            for (j = i; j >= gap && s->arr[j - gap] > temp && !s->quit; j -= gap) {
                s->current_idx = j;
                s->current_idx2 = j - gap;
                s->comparisons++;
                s->arr[j] = s->arr[j - gap];
                s->swaps++;
                on_sort_step(s);
            }
            s->arr[j] = temp;
        }
    }
    s->sorted_until = s->n;
}

static void cocktail_sort(SortState *s) {
    bool swapped = true;
    int start = 0;
    int end = s->n - 1;

    while (swapped && !s->quit) {
        swapped = false;
        for (int i = start; i < end && !s->quit; i++) {
            s->current_idx = i;
            s->current_idx2 = i + 1;
            s->comparisons++;
            on_sort_step(s);
            if (s->arr[i] > s->arr[i + 1]) {
                swap(&s->arr[i], &s->arr[i + 1]);
                s->swaps++;
                swapped = true;
                on_sort_step(s);
            }
        }
        end--;
        s->sorted_until = end + 1;
        if (!swapped || s->quit) break;

        swapped = false;
        for (int i = end - 1; i >= start && !s->quit; i--) {
            s->current_idx = i;
            s->current_idx2 = i + 1;
            s->comparisons++;
            on_sort_step(s);
            if (s->arr[i] > s->arr[i + 1]) {
                swap(&s->arr[i], &s->arr[i + 1]);
                s->swaps++;
                swapped = true;
                on_sort_step(s);
            }
        }
        start++;
    }
    s->sorted_until = s->n;
}

static void gnome_sort(SortState *s) {
    int i = 0;
    while (i < s->n && !s->quit) {
        s->current_idx = i;
        on_sort_step(s);
        if (i == 0 || s->arr[i] >= s->arr[i - 1]) {
            i++;
        } else {
            s->comparisons++;
            swap(&s->arr[i], &s->arr[i - 1]);
            s->swaps++;
            on_sort_step(s);
            i--;
        }
    }
    s->sorted_until = s->n;
}

static void radix_sort(SortState *s) {
    int max_val = s->arr[0];
    for (int i = 1; i < s->n; i++)
        if (s->arr[i] > max_val) max_val = s->arr[i];

    int *output = (int *)malloc(s->n * sizeof(int));

    for (int exp = 1; max_val / exp > 0 && !s->quit; exp *= 10) {
        int count[10] = {0};

        for (int i = 0; i < s->n; i++)
            count[(s->arr[i] / exp) % 10]++;

        for (int i = 1; i < 10; i++)
            count[i] += count[i - 1];

        for (int i = s->n - 1; i >= 0 && !s->quit; i--) {
            int idx = (s->arr[i] / exp) % 10;
            output[count[idx] - 1] = s->arr[i];
            count[idx]--;
            s->current_idx = i;
            on_sort_step(s);
        }

        for (int i = 0; i < s->n && !s->quit; i++) {
            s->arr[i] = output[i];
            s->current_idx = i;
            on_sort_step(s);
        }
    }

    free(output);
    s->sorted_until = s->n;
}

// ----------------------------------------------------------------

void run_sort(SortState *s) {
    s->sorting = true;
    s->sorted = false;
    s->comparisons = 0;
    s->swaps = 0;
    s->current_idx = -1;
    s->current_idx2 = -1;
    s->sorted_until = 0;
    s->quit = false;
    on_sort_step(s);

    switch (s->algo) {
        case ALGO_BUBBLE:    bubble_sort(s);    break;
        case ALGO_SELECTION: selection_sort(s);  break;
        case ALGO_INSERTION: insertion_sort(s);  break;
        case ALGO_QUICK:     quick_sort(s);      break;
        case ALGO_MERGE:     merge_sort(s);      break;
        case ALGO_HEAP:      heap_sort(s);       break;
        case ALGO_SHELL:     shell_sort(s);      break;
        case ALGO_COCKTAIL:  cocktail_sort(s);   break;
        case ALGO_GNOME:     gnome_sort(s);      break;
        case ALGO_RADIX:     radix_sort(s);      break;
        default: break;
    }

    s->sorting = false;
    s->sorted = true;
    s->current_idx = -1;
    s->current_idx2 = -1;
    s->sorted_until = s->n;
    on_sort_step(s);
}
