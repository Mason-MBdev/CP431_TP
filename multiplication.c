#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Compare function, for qsort.
int compare(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    // Get N from command line argument, default to 10 for safety if nothing is provided.
    int N = 10;
    if (argc > 1) {
        N = atoi(argv[1]);
    } else if (world_rank == 0) {
        printf("No value provided for N, using default N=10\n");
    }
    
    // Broadcast N to all processes
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Calculate total number of products to distribute evenly
    int total_pairs = (N * (N + 1)) / 2;  // Only compute upper triangular matrix (i <= j) because of symmetry.
    int pairs_per_proc = total_pairs / world_size;
    int remainder = total_pairs % world_size;
    
    // Determine start and end indexes
    int start_idx = world_rank * pairs_per_proc + (world_rank < remainder ? world_rank : remainder);
    int end_idx = start_idx + pairs_per_proc + (world_rank < remainder ? 1 : 0) - 1;
    
    if (world_rank == 0) {
        printf("Computing M(%d) with %d processes...\n", N, world_size);
    }

    // Local storage of products found by the process.
    int *products = (int *)malloc(N * N * sizeof(int));
    if (products == NULL) {
        printf("Memory allocation failed\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Local count for the number of products in the process.
    int count = 0;

    // Compute all products for assigned portion.
    int global_idx = 0;
    for (int i = 1; i <= N; i++) {
        for (int j = i; j <= N; j++) {
            if (global_idx >= start_idx && global_idx <= end_idx) {
                products[count++] = i * j;
            }
            global_idx++;
        }
    }

    if (world_rank == 0) {
        printf("Main process has gathered all computed products\n");
    }

    // Gather all products to the root process
    int *all_counts = NULL;
    if (world_rank == 0) {
        all_counts = (int *)malloc(world_size * sizeof(int));
    }
    
    MPI_Gather(&count, 1, MPI_INT, all_counts, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    // Compute global results
    if (world_rank == 0) {
        // Calculate displacements and total size
        int total_size = 0;
        int *displacements = (int *)malloc(world_size * sizeof(int));
        
        for (int i = 0; i < world_size; i++) {
            displacements[i] = total_size;
            total_size += all_counts[i];
        }
        
        // Allocate memory for all products
        int *all_products = (int *)malloc(total_size * sizeof(int));
        
        // Gather all products from each process
        MPI_Gatherv(products, count, MPI_INT, 
                   all_products, all_counts, displacements, 
                   MPI_INT, 0, MPI_COMM_WORLD);
        
        // Sort all products
        qsort(all_products, total_size, sizeof(int), compare);
        
        // Count unique elements
        int unique_count = 0;
        int prev = -1;
        
        for (int i = 0; i < total_size; i++) {
            if (i == 0 || all_products[i] != prev) {
                unique_count++;
            }
            prev = all_products[i];
        }
        
        printf("M(%d) = %d\n", N, unique_count);
        
        free(all_products);
        free(displacements);
        free(all_counts);
    } else {
        MPI_Gatherv(products, count, MPI_INT, 
                   NULL, NULL, NULL, 
                   MPI_INT, 0, MPI_COMM_WORLD);
    }
    
    // Clean up
    free(products);
    MPI_Finalize();
    return 0;
}
