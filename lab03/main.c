#include <mpi.h>
#include <stdio.h>

void
task1(void)
{
    int world_size = 0;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int world_rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    int name_len = 0;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    MPI_Get_processor_name(processor_name, &name_len);

    printf("Processor %s, rank %d out of %d processors\n",
            processor_name, world_rank, world_size);
}

void
task2(void)
{
    int world_size = 0;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int world_rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    if (world_rank == 0)
        for (int i = 1; i < world_size; ++i)
        {
            int recv_rank = 0;
            MPI_Status status = { 0 };

            MPI_Recv(&recv_rank, 1, MPI_INT, MPI_ANY_SOURCE,
                MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            printf("Message from %d\n", recv_rank);
        }
    else
        MPI_Send(&world_rank, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
}

int
main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    task1();
    task2();

    MPI_Finalize();
    return 0;
}
