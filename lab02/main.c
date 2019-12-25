#include <omp.h>

#include <stdio.h>
#include <stdlib.h>

#define NMAX 1000
#define CHUNK_SIZE 100

void
task1(void)
{
    puts("task1:");

    double t1 = omp_get_wtime();
    #pragma omp parallel
    {
        printf("Hello world!\n");
    }
    double t2 = omp_get_wtime();

    printf("num_proc=%d, max_threads=%d\n",
            omp_get_num_procs(), omp_get_max_threads());
    printf("dynamic=%d, nested=%d\n",
            omp_get_dynamic(), omp_get_nested());

    double dt = t2 - t1;
    printf("dt=%lf\n", dt);
}

void
task2(void)
{
    puts("task2:");

    size_t i = 0, j = 0;
    double a[NMAX][NMAX];

    #pragma omp parallel shared(a) private(i, j)
    {
        #pragma omp for schedule(dynamic, CHUNK_SIZE)
        for (i = 0; i < NMAX; ++i)
            for (j = 0; j < NMAX; ++j)
                a[i][j] = rand() % 23;
    }

    double sum = 0, smax = 0, total = 0;

#ifndef _LAB_02_CRIT_
    omp_lock_t lock;
    omp_init_lock(&lock);
#endif

    #pragma omp parallel shared(a) private(i, j, sum)
    {
        #pragma omp for ordered \
                schedule(dynamic, CHUNK_SIZE) reduction(+:total)
        for (i = 0; i < NMAX; ++i)
        {
            sum = 0;
            for (j = 0; j < NMAX; ++j)
                sum += a[i][j];

            if (sum > smax)
#ifdef _LAB_02_CRIT_
                #pragma omp critical (smax_calc)
#else
            {
                omp_set_lock(&lock);
#endif
                if (sum > smax)
                    smax = sum;
#ifndef _LAB_02_CRIT_
                omp_unset_lock(&lock);
            }
#endif

            #pragma omp atomic
            total += sum;

            #pragma omp ordered
            printf("th=%d; s[%lu] = %lf\n", omp_get_thread_num(), i, sum);
        }
    }

#ifndef _LAB_02_CRIT_
    omp_destroy_lock(&lock);
#endif

    printf("smax=%lf, total=%lf\n", smax, total);
}

void
task3(void)
{
    puts("task3:");

    double min_val = 0;
    const size_t n_max = NMAX / 4;

    size_t i = 0, j = 0;
    double a[n_max][n_max], b[n_max][n_max];

    #pragma omp parallel shared(a) private(i, j)
    {
        #pragma omp for schedule(dynamic, CHUNK_SIZE)
        for (i = 0; i < n_max; ++i)
            for (j = 0; j < n_max; ++j)
                a[i][j] = rand() % 23;
    }

    #pragma omp parallel shared(a, b) private(i, j)
    {
        #pragma omp sections
        {
            #pragma omp section
            for (i = 0; i < n_max; ++i)
                for (j = 0; j < n_max; ++j)
                {
                    if (a[i][j] < min_val)
                        if (a[i][j] < min_val)
                            min_val = a[i][j];
                }

            #pragma omp section
            for (i = 0; i < n_max; ++i)
                for (j = 0; j < n_max; ++j)
                    b[i][j] = a[i][j];
        }
    }

    printf("min=%lf\n", min_val);
}

int
main(void)
{
    task1();
    task2();
    task3();

    return 0;
}
