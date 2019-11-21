#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <knapsack.h>

int
main(int argc, const char **argv)
{
    assert(argc == 3);

    const char *src_path = argv[1];
    const char *dst_path = argv[2];

    FILE *src_file = fopen(src_path, "r");
    FILE *dst_file = fopen(dst_path, "w");

    items_t items = { 0 };
    knapsack_t knapsack = { 0 };

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

    err = pack_knapsack(&knapsack, &items);
    if (err != OK)
        goto out;

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
