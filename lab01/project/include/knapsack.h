#ifndef LAB01_KNAPSACK_H
#define LAB01_KNAPSACK_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "error.h"

typedef uint64_t int_t;

typedef int_t value_t;
typedef int_t weight_t;

typedef struct
{
    value_t  value;
    weight_t weight;
} item_t;

typedef item_t* item_ptr_t;

typedef struct
{
    size_t count;
    size_t capacity;

    item_ptr_t arr;

    value_t  total_value;
    weight_t total_weight;
} items_t;

#define INITIAL_ITEMS_CAPACITY 1

__always_inline error_t
init_items(items_t *items)
{
    memset(items, 0, sizeof(items_t));

    items->capacity = INITIAL_ITEMS_CAPACITY;
    items->arr = malloc(items->capacity * sizeof(item_t));
    if (!items->arr)
        return MEM_ERR;

    return OK;
}

#define drop_items(x)    \
    do                   \
    {                    \
        free((x)->arr);  \
        (x)->arr = NULL; \
    }                    \
    while (0);

error_t
read_items_info(FILE *f, items_t *items);

error_t
write_items_info(FILE *f, const items_t *items);

error_t
add_item_to_items(items_t *items, const item_t *item);

error_t
add_random_items_to_items(items_t *items, int_t num_items,
                          int_t item_value_min, int_t item_value_max,
                          int_t item_weight_min, int_t item_weight_max);

typedef struct
{
    items_t  items;
    weight_t max_weight;
} knapsack_t;

__always_inline error_t
init_knapsack(knapsack_t *knapsack)
{
    memset(knapsack, 0, sizeof(knapsack_t));
    return init_items(&knapsack->items);
}

__always_inline void
drop_knapsack(knapsack_t *knapsack)
{
    drop_items(&knapsack->items);
}

error_t
read_knapsack_info(FILE *f, knapsack_t *knapsack);

error_t
write_knapsack_info(FILE *f, const knapsack_t *knapsack);

error_t
add_item_to_knapsack(knapsack_t *knapsack, const item_t *item);

error_t
pack_knapsack(knapsack_t *knapsack, double *dt, const items_t *items);

error_t
pack_knapsack_omp(knapsack_t *knapsack, double *dt, const items_t *items);

error_t
pack_knapsack_mpi(knapsack_t *knapsack, double *dt, const items_t *items);

#endif //LAB01_KNAPSACK_H
