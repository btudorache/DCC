#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // Added for gethostname

#define BUFFER_SIZE 1024
#define STAGE_COUNT 10000

int main(int argc, char** argv) {
    close(STDOUT_FILENO);

    int rank, size;
    char hostname[256];

    // Initialize MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    gethostname(hostname, sizeof(hostname));

    // Allocate send and receive buffers for Alltoall
    int *send_data = (int*)malloc(size * sizeof(int));
    int *recv_data = (int*)malloc(size * sizeof(int));

    printf("Rank %d started on host %s\n", rank, hostname);

    for(int stage = 0; stage < STAGE_COUNT; stage++) {
        // Prepare data to send - unique for each destination
        for(int i = 0; i < size; i++) {
            send_data[i] = rank * 100 + i + (stage * 1000);
        }

        // Perform all-to-all communication
        MPI_Alltoall(send_data, 1, MPI_INT,
                     recv_data, 1, MPI_INT,
                     MPI_COMM_WORLD);

        // Print received data
        printf("Stage %d - Rank %d received: ", stage + 1, rank);
        for(int i = 0; i < size; i++) {
            printf("%d ", recv_data[i]);
        }
        printf("\n");

        // Synchronize before next stage
        MPI_Barrier(MPI_COMM_WORLD);
    }

    // Clean up
    free(send_data);
    free(recv_data);
    
    MPI_Finalize();
    return 0;
}