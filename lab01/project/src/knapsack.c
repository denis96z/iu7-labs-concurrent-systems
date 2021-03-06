#include <errno.h>
#include <assert.h>
#include <stdlib.h>

#include <omp.h>
#include <mpi.h>

#include <macro.h>
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

#define rand_range(min, max) \
    ((rand() % ((max) - (min))) + (min))

error_t
add_random_items_to_items(items_t *items, int_t num_items,
                          int_t item_value_min, int_t item_value_max,
                          int_t item_weight_min, int_t item_weight_max)
{
    for (int_t i = 0; i < num_items; ++i)
    {
        item_t item = {
                .value  = rand_range(item_value_min, item_value_max),
                .weight = rand_range(item_weight_min, item_weight_max),
        };

        error_t err = add_item_to_items(items, &item);
        if (err != OK)
            return err;
    }
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

//#define __LOG_STAT__
#define printnl(n)                     \
    do {                               \
        for (size_t i = 0; i < n; ++i) \
            printf("%s", "\n");        \
    } while(0);

error_t
pack_knapsack(knapsack_t *knapsack, double *dt, const items_t *items)
{
    assert(knapsack && items);

    int_t **pm = NULL;
    const size_t num_rows = items->count + 1;
    const size_t num_cols = knapsack->max_weight + 1;

    error_t err = alloc_matrix(&pm, num_rows, num_cols, 1);
    if (err != OK)
        return err;

#ifdef __LOG_STAT__
    puts("Task stat:");
    printf("num_rows=%lu, num_cols=%lu\n", num_rows, num_cols);
    printnl(1);
#endif

    double t1 = omp_get_wtime();

    for (int_t i = 1; i < num_rows; i++)
    {
        for (int_t j = 0; j < num_cols; j++)
        {
            pm[i][j] = pm[i - 1][j];

            if ((j >= items->arr[i - 1].weight) &&
                    (pm[i][j] < pm[i - 1][j - items->arr[i - 1].weight] + items->arr[i - 1].value))
            {
                pm[i][j] = pm[i - 1][j - items->arr[i - 1].weight] + items->arr[i - 1].value;
            }
        }
    }

    size_t w = knapsack->max_weight;
    for (int_t n = items->count; n > 0; --n)
    {
        if (pm[n][w] != pm[n - 1][w])
        {
            w -= items->arr[n - 1].weight;
            add_item_to_knapsack(knapsack, &items->arr[n - 1]);
        }
    }

    double t2 = omp_get_wtime();
    *dt = t2 - t1;

    free_matrix(pm, num_rows);
    return OK;
}

error_t
pack_knapsack_omp(knapsack_t *knapsack, double *dt, const items_t *items)
{
    assert(knapsack && items);

    int_t **pm = NULL;
    const size_t num_rows = items->count + 1;
    const size_t num_cols = knapsack->max_weight + 1;

    error_t err = alloc_matrix(&pm, num_rows, num_cols, 1);
    if (err != OK)
        return err;

    omp_set_dynamic(1);

#ifdef __LOG_STAT__
    puts("Task stat:");
    printf("num_rows=%lu, num_cols=%lu\n", num_rows, num_cols);
    printnl(1);

    puts("OpenMP stat:");
    printf("num_proc=%d, max_threads=%d\n",
           omp_get_num_procs(), omp_get_max_threads());
    printf("dynamic=%d, nested=%d\n",
           omp_get_dynamic(), omp_get_nested());
    printnl(1);
#endif

    int_t j = 0;
    int_t prev_row_value = 0;

    double t1 = omp_get_wtime();

    for (int_t i = 1; i < num_rows; ++i)
    {
        #pragma omp parallel default(shared) private(j, prev_row_value)
        {
            #pragma omp for
            for (j = 0; j < num_cols; ++j)
            {
                pm[i][j] = pm[i - 1][j];
                prev_row_value = pm[i - 1][j - items->arr[i - 1].weight] +
                        items->arr[i - 1].value;

                if ((j >= items->arr[i - 1].weight) && (pm[i][j] < prev_row_value))
                    pm[i][j] = prev_row_value;
            }
        }
    }

    size_t w = knapsack->max_weight;
    for (int_t n = items->count; n > 0; --n)
    {
        if (pm[n][w] != pm[n - 1][w])
        {
            w -= items->arr[n - 1].weight;
            add_item_to_knapsack(knapsack, &items->arr[n - 1]);
        }
    }

    double t2 = omp_get_wtime();
    *dt = t2 - t1;

    free_matrix(pm, num_rows);
    return OK;
}

#define ROW 32
#define COL 256

#define min(x, y) \
    (((x) < (y)) ? (x) : (y))

void
solve_mpi(int n, int c,
          int rows, const items_t *items,
          int start, int rank, int size);

error_t
pack_knapsack_mpi(knapsack_t *knapsack, double *dt, const items_t *items)
{
    int size, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for (int i = 0; i < items->count; i += ROW)
    {
        int rows  = min(ROW, items->count - i);
        if ((i / ROW) % size == rank)
            solve_mpi(items->count, knapsack->max_weight,
                      rows, items, i, rank, size);
    }
}

void
solve_mpi(int n, int c,
          int rows, const items_t *items,
          int start, int rank, int size)
{
    int recv_rank = (rank-1)%size;
    int send_rank = (rank+1)%size;
    if (start == 0)
    {
        int i, j;
        int **mp = malloc(sizeof(int [rows][c]));

        for (j = 0; j < c; j += COL)
        {
            int cols = min(COL, c-j);
            int k;
            for (k = j; k < j + cols; k++) {
                if (items->arr[0].weight > k) {
                    mp[0][k] = 0;
                } else {
                    mp[0][k] = items->arr[0].value;
                }
            }
            for (i = 1; i < rows; i++) {
                for (k = j; k < j + cols; k++) {
                    if ( (k<items->arr[i].weight) ||
                         (mp[i-1][k] >= mp[i-1][k-items->arr[i].weight] + items->arr[i].value)) {
                        mp[i][k] = mp[i-1][k];
                    } else {
                        mp[i][k] = mp[i-1][k-items->arr[i].weight] + items->arr[i].value;
                    }
                }
            }
            MPI_Send(&mp[rows-1][j], cols, MPI_INT, send_rank, j, MPI_COMM_WORLD);
        }

        free(mp);
    } else {
        int **mp = malloc(sizeof(int [rows+1][c]));

        int i, j;
        for (j = 0; j < c; j += COL) {
            int cols = min(COL, c-j);
            int k;
            MPI_Recv(&mp[0][j], cols, MPI_INT, recv_rank, j, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (i = 1; i <= rows; i++) {
                for (k = j; k < j + cols; k++) {
                    int ni = i-1;
                    if ( (k<items->arr[ni].weight) ||
                         (mp[i-1][k] >= mp[i-1][k-items->arr[ni].weight] + items->arr[ni].value)) {
                        mp[i][k] = mp[i-1][k];
                    } else {
                        mp[i][k] = mp[i-1][k-items->arr[ni].weight] + items->arr[ni].value;
                    }
                }
            }

            if (start + rows == n && j + cols == c) {
                //TODO handle max value
            } else if (start + rows != n){
                MPI_Send(&mp[rows][j], cols, MPI_INT, send_rank, j, MPI_COMM_WORLD);
            }
        }

        free(mp);
    }
}
