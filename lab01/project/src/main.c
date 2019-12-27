#include <stdio.h>
#include <stdlib.h>

#include <knapsack.h>

#define IS_TEST(mode) \
    ((mode)[0] == 't')

#define TEST_MIN 3
#define TEST_MAX 20

__always_inline int_t
intpow(size_t x, size_t y)
{
    size_t r = x;
    for (size_t i = 1; i < y; ++i)
        r *= x;
    return r;
}

typedef error_t (*pack_func_t)(knapsack_t *, double *, const items_t *);

int
main(int argc, const char **argv)
{
    if (argc != 4)
    {
        printf("%s mode source destination", argv[0]);
        return 1;
    }

    const char *mode     = argv[1];
    const char *src_path = argv[2];
    const char *dst_path = argv[3];

    FILE *src_file = fopen(src_path, "r");
    FILE *dst_file = IS_TEST(mode) ? fopen(dst_path, "w"): NULL;

    items_t items = { 0 };
    knapsack_t knapsack = { 0 };

    error_t err = OK;
    if (!src_file || (!IS_TEST(mode) && !dst_file))
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
    switch (mode[0])
    {
        case 's':
            err = pack_knapsack(&knapsack, &dt, &items);
            break;

        case 'o':
            err = pack_knapsack_omp(&knapsack, &dt, &items);
            break;

        case 't':
            for (size_t i = TEST_MIN; i <= TEST_MAX; ++i)
            {
                double ts = 0, to = 0;
                weight_t max_weight = (weight_t)intpow(2, i);

                for (size_t j = 1; j <= 2; ++j)
                {
                    knapsack_t tk;
                    if ((err = init_knapsack(&tk)) != OK)
                        goto out;
                    tk.max_weight = max_weight;

                    double *dp = NULL;
                    pack_func_t pf = NULL;

                    switch (j)
                    {
                        case 1:
                            dp = &ts;
                            pf = pack_knapsack;
                            break;

                        case 2:
                            dp = &to;
                            pf = pack_knapsack_omp;
                            break;
                    }

                    if ((err = pf(&tk, dp, &items)) != OK)
                    {
                        drop_knapsack(&tk);
                        goto out;
                    }

                    drop_knapsack(&tk);
                }

                printf("TEST=%lu, WEIGHT=%lu, SINGLE=%lf, OPENMP=%lf\n",
                        i, max_weight, ts, to);
            }
            break;

        default:
            err = OK;
    }
    if (err != OK)
        goto out;

    if (!IS_TEST(mode))
    {
        printf("Task complete. Duration = %lf\n", dt);
        err = write_knapsack_info(dst_file, &knapsack);
    }

out:
    drop_knapsack(&knapsack);
    drop_items(&items);

    dst_file ? fclose(dst_file):0;
    src_file ? fclose(src_file):0;

    if (err != OK)
        return ERR_TO_RET_CODE(err);

    return 0;
}
