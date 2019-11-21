#include <errno.h>
#include <assert.h>
#include <stdlib.h>

#include <knapsack.h>

#define BUF_SIZE 16

static char
file_str_buff[BUF_SIZE];

static error_t
read_unsigned_long(FILE *f, unsigned long *x);

static error_t
parse_unsigned_long(unsigned long *x, const char *buff);

static error_t
read_item_info(FILE *f, item_t *item);

#define ULPTR(x) \
    ((unsigned long *)(&x))

#define ITEMPTR(x) \
    ((item_t *)(&x))

error_t
read_knapsack_info(FILE *f, knapsack_t *knapsack)
{
    assert(f && knapsack);
    return read_unsigned_long(f, ULPTR(knapsack->max_weight));
}

error_t
write_knapsack_info(FILE *f, const knapsack_t *knapsack)
{
    assert(f && knapsack);
    return write_items_info(f, &knapsack->items);
}

error_t
read_items_info(FILE *f, items_t *items)
{
    assert(f);

    error_t err = read_unsigned_long(f, ULPTR(items->capacity));
    if (err != OK)
        return err;

    item_ptr_t new_arr = (item_t*)realloc(items->arr, items->capacity * sizeof(item_t));
    if (!new_arr)
	    return MEM_ERR;

    for (size_t i = 0; i < items->capacity; ++i)
    {
        err = read_item_info(f, ITEMPTR(new_arr[i]));
        if (err != OK)
        {
            drop_items(items);
            return err;
        }
    }

    items->arr = new_arr;
    items->count = items->capacity;

    return OK;
}

#define INT_FMT "%lu"

#define write_and_check(fmt, ...)               \
    do {                                        \
        if (fprintf(f, fmt, ##__VA_ARGS__) < 1) \
            return FIO_ERR;                     \
    } while(0);

error_t
write_items_info(FILE *f, const items_t *items)
{
    assert(f && items);

    error_t err = OK;
    write_and_check(INT_FMT" "INT_FMT"\n",
            items->total_weight, items->total_value);

    write_and_check(INT_FMT"\n", items->count);
    for (size_t i = 0; i < items->count; ++i)
    {
        write_and_check(INT_FMT" "INT_FMT"\n",
                items->arr[i].weight, items->arr[i].value);
    }

    return OK;
}

error_t
add_item_to_items(items_t *items, const item_t *item)
{
    assert(items && item);

    if (items->count == items->capacity)
    {
        size_t new_cap = 2 * items->capacity;
        item_t *new_arr = realloc(items->arr, new_cap * sizeof(item_t));
        if (!new_arr)
            return MEM_ERR;

        items->arr = new_arr;
        items->capacity = new_cap;
    }

    items->arr[items->count++] = *item;
    items->total_value += item->value;
    items->total_weight += item->weight;

    return OK;
}

static error_t
read_unsigned_long(FILE *f, unsigned long *x)
{
    assert(f && x);

    if (!fgets(file_str_buff, BUF_SIZE, f))
        return FIO_ERR;

    return parse_unsigned_long(x, file_str_buff);
}

static error_t
parse_unsigned_long(unsigned long *x, const char *buff)
{
    assert(x);

    errno = 0;
    *x = strtoul(buff, NULL, 10);
    if (errno != 0)
        return FMT_ERR;

    return OK;
}

static error_t
read_item_info(FILE *f, item_t *item)
{
    assert(f);

    if (!fgets(file_str_buff, BUF_SIZE, f))
        return FIO_ERR;

    size_t idx = 0;
    for (size_t i = 0; file_str_buff[i]; ++i)
        switch (file_str_buff[i])
        {
            case ' ':
                file_str_buff[idx = i] = '\0';
                break;

            case '\r': case '\n':
                file_str_buff[i] = '\0';
                goto out;
        }
out:;

    error_t err = parse_unsigned_long(ULPTR(item->weight), file_str_buff);
    if (err != OK)
        return err;

    err = parse_unsigned_long(ULPTR(item->value), file_str_buff + idx + 1);
    if (err != OK)
        return err;

    return OK;
}

error_t
add_item_to_knapsack(knapsack_t *knapsack, const item_t *item)
{
    assert(knapsack && item);
    return add_item_to_items(&knapsack->items, item);
}

static void
free_matrix(int_t **m, size_t rc);

#define __alloc_rows__(m, rc, acall)      \
    do {                                  \
        for (size_t i = 0; i < (rc); ++i) \
        {                                 \
            (m)[i] = acall;               \
            if (!((m)[i]))                \
            {                             \
                free_matrix((m), rc - i); \
                return MEM_ERR;           \
            }                             \
        }                                 \
    } while(0)

static error_t
alloc_matrix(int_t ***m, size_t rc, size_t cc, int clean)
{
    assert(m && rc && cc);

    int_t **mn = (int_t **)calloc(sizeof(int_t *), rc);
    if (!mn)
        return MEM_ERR;

    if (clean)
        __alloc_rows__(mn, rc, calloc(cc, sizeof(int_t)));
    else
        __alloc_rows__(mn, rc, malloc(cc * sizeof(int_t)));

    *m = mn;
    return OK;
}

static void
free_matrix(int_t **m, size_t rc)
{
    assert(m && rc);

    for (size_t i = 0; i < rc; ++i)
        free(m[i]);

    free(m);
}

static void
put_items_into_knapsack(knapsack_t *knapsack, const items_t *items,
                        int_t **pm, size_t k, size_t w);

static inline int_t
max_int(int_t a, int_t b)
{
    return a > b ? a : b;
}

#define max(a, b) \
    max_int(a, b)

error_t
pack_knapsack(knapsack_t *knapsack, const items_t *items)
{
    int_t **pm = NULL;

    const size_t num_rows = items->count + 1;
    const size_t num_cols = knapsack->max_weight + 1;

    error_t err = alloc_matrix(&pm, num_rows, num_cols, 1);
    if (err != OK)
        return err;

    for (size_t k = 1; k < num_rows; ++k)
        for (size_t w = 1; w < num_cols; ++w)
        {
            value_t vk = items->arr[k - 1].value;
            weight_t wk = items->arr[k - 1].weight;

            if (w >= wk)
                pm[k][w] = max(pm[k - 1][w], pm[k - 1][w - wk] + vk);
            else
                pm[k][w] = pm[k - 1][w];
        }

    put_items_into_knapsack(knapsack, items, pm,
                            items->count, knapsack->max_weight);

    free_matrix(pm, num_rows);
    return OK;
}

static void
put_items_into_knapsack(knapsack_t *knapsack, const items_t *items,
                        int_t **pm, size_t k, size_t w)
{
    if (pm[k][w] == 0)
        return;
    if (pm[k - 1][w] == pm[k][w])
        put_items_into_knapsack(knapsack, items, pm, k - 1, w);
    else
    {
        put_items_into_knapsack(knapsack, items, pm, k - 1, w - items->arr[k - 1].weight);
        add_item_to_knapsack(knapsack, &items->arr[k - 1]);
    }
}
