#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <mpi.h>

#include <macro.h>
#include <knapsack.h>

#define OMP_FLAG "--omp"
#define MPI_FLAG "--mpi"

#define TEST_FLAG "--test"

#define TASK_NUM_ARGS 4
#define TEST_NUM_ARGS 12

#define __check_mode__(mode, tested) \
    (strcmp((mode), (tested)) == 0)
#define IS_OMP(mode) \
    __check_mode__(mode, OMP_FLAG)
#define IS_MPI(mode) \
    __check_mode__(mode, MPI_FLAG)
#define IS_TEST(mode) \
    __check_mode__(mode, TEST_FLAG)

int
do_task(int argc, char *argv[]);

int
do_test(int argc, char *argv[]);

int
main(int argc, char *argv[])
{
    if (argc < 2)
        goto usage;

    const char *mode = argv[1];
    if (IS_TEST(mode))
    {
        if (argc != TEST_NUM_ARGS)
            goto usage;
    }
    else
    {
        if (argc != TASK_NUM_ARGS)
            goto usage;
    }

    return IS_TEST(mode)
        ? do_test(argc, argv)
        : do_task(argc, argv);

usage:
    puts("Usage:");
    printf("%s [--mpi|--omp] source destination\n", argv[0]);
    printf("%s --test nmin nmax nstep wmin wmax wstep vimin vimax wimin wimax\n", argv[0]);

    return 1;
}

typedef error_t (*pack_func_t)(knapsack_t *, double *, const items_t *);

int
do_task(int argc, char *argv[])
{
    const char *mode     = argv[1];
    const char *src_path = argv[2];
    const char *dst_path = argv[3];

    if (IS_MPI(mode))
        MPI_Init(&argc, &argv);

    FILE *src_file = fopen(src_path, "r");
    FILE *dst_file = fopen(dst_path, "w");

    items_t items = new(items_t);
    knapsack_t knapsack = new(knapsack_t);

    error_t err = OK;
    if (!src_file || !dst_file)
    {
        err = ARG_ERR;
        goto out;
    }

    err = init_items(&items);
    if (err != OK)
        goto out;

    err = init_knapsack(&knapsack);
    if (err != OK)
        goto out;

    err = read_knapsack_info(src_file, &knapsack);
    if (err != OK)
        goto out;

    err = read_items_info(src_file, &items);
    if (err != OK)
        goto out;

    double dt = 0;
    pack_func_t pack_knapsack_func = pack_knapsack;

    if (IS_OMP(mode))
        pack_knapsack_func = pack_knapsack_omp;
    elif (IS_MPI(mode))
        pack_knapsack_func = pack_knapsack_mpi;

    err = pack_knapsack_func(&knapsack, &dt, &items);
    if (err != OK)
        goto out;

    printf("Task complete. Duration = %lf\n", dt);
    err = write_knapsack_info(dst_file, &knapsack);

out:
    drop_knapsack(&knapsack);
    drop_items(&items);

    dst_file ? fclose(dst_file):0;
    src_file ? fclose(src_file):0;

    if (err != OK)
        return ERR_TO_RET_CODE(err);

    return 0;
}

#define parse_int_t(str) \
    strtoul((str), NULL, 10)

#define parse_arg_int_t(name, idx)          \
    const char *name ## _str = argv[idx];   \
    int_t name = parse_int_t(name ## _str);

#define parse_args_int_t_bounds(name, idx1)   \
    parse_arg_int_t(name ## _min,  idx1);     \
    parse_arg_int_t(name ## _max,  idx1 + 1); \

#define parse_args_int_t_range(name, idx1)    \
    parse_args_int_t_bounds(name, idx1);      \
    parse_arg_int_t(name ## _step, idx1 + 2);

int
do_test(int argc, char *argv[])
{
    error_t err = OK;
    assert(argc == TEST_NUM_ARGS);

    parse_args_int_t_range(num_items, 2);
    parse_args_int_t_range(max_weight, 5);

    parse_args_int_t_bounds(item_value, 8);
    parse_args_int_t_bounds(item_weight, 10);

    for (int_t w = max_weight_min; w <= max_weight_max; w += max_weight_step)
    {
        for (size_t n = num_items_min; n <= num_items_max; n += num_items_step)
        {
            double ts = 0, to = 0;
            for (size_t j = 1; j <= 2; ++j)
            {
                items_t items = new(items_t);
                knapsack_t knapsack = new(knapsack_t);

                if ((err = init_items(&items)) != OK)
                    goto out;
                if ((err = init_knapsack(&knapsack)) != OK)
                    goto out;

                knapsack.max_weight = w;
                err = add_random_items_to_items(&items, n,
                                                item_value_min, item_value_max,
                                                item_weight_min, item_weight_max);
                if (err != OK)
                    goto out;

                double *dp = NULL;
                pack_func_t pack_func = NULL;

                switch (j)
                {
                    case 1:
                        dp = &ts;
                        pack_func = pack_knapsack;
                        break;

                    case 2:
                        dp = &to;
                        pack_func = pack_knapsack_omp;
                        break;

                    default:
                        //TODO handle
                        goto out;
                }

                err = pack_func(&knapsack, dp, &items);

            out:
                drop_items(&items);
                drop_knapsack(&knapsack);

                if (err != OK)
                    return ERR_TO_RET_CODE(err);
            }

            printf("%lu %lf %lf\n", w, ts, to);
        }
    }

    return 0;
}
