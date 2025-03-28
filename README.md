# Parallel Programming, Multiplication Table Analysis

A parallel implementation for computing the number of unique elements in N×N multiplication tables using the Message Passing Interface.

## Problem Statement

An N×N multiplication table is defined as a matrix where each element A_ij = i × j, for i, j = 1, ..., N.

Key observations:
- The multiplication table is symmetric
- The main diagonal contains squares: 1², 2², ..., N²
- Some elements above the main diagonal are repeated

We define M(N) as the count of unique elements in the N×N multiplication table.

For example:
- M(2) = 3 (unique elements: 1, 2, 4)
- M(3) = 6 (unique elements: 1, 2, 3, 4, 6, 9)
- M(4) = 9 (unique elements: 1, 2, 3, 4, 6, 8, 9, 12, 16)
- M(10) = 42

## Implementation

The algorithm follows these four steps:
1. Computation of only the upper triangular part of the matrix
2. Even distribution of workload across MPI processes
3. Efficient gathering and merging of results
4. Sorting and unique element counting in the main process

### Key Components of the Code

#### 1. Work Distribution

The algorithm distributes the computation evenly across all processes by calculating the total number of pairs in the upper triangular matrix and then dividing the work:

```c
// Calculate total number of products to distribute evenly
int total_pairs = (N * (N + 1)) / 2;  // Only compute upper triangular matrix (i <= j)
int pairs_per_proc = total_pairs / world_size;
int remainder = total_pairs % world_size;

// Determine start and end indices for this process
int start_idx = world_rank * pairs_per_proc + (world_rank < remainder ? world_rank : remainder);
int end_idx = start_idx + pairs_per_proc + (world_rank < remainder ? 1 : 0) - 1;
```

#### 2. Computing Products

Each process computes only its assigned portion of the multiplication table:

```c
// Compute all products for assigned portion
int global_idx = 0;
for (int i = 1; i <= N; i++) {
    for (int j = i; j <= N; j++) {  // Only compute upper triangular matrix (i <= j)
        if (global_idx >= start_idx && global_idx <= end_idx) {
            products[count++] = i * j;
        }
        global_idx++;
    }
}
```

#### 3. Gathering Results

The main process gathers products from all processes and merges them:

```c
// Gather all products from each process
MPI_Gatherv(products, count, MPI_INT, 
           all_products, all_counts, displacements, 
           MPI_INT, 0, MPI_COMM_WORLD);

// Sort all products
qsort(all_products, total_size, sizeof(int), compare);
```

#### 4. Counting Unique Elements

After sorting, counting unique elements is straightforward:

```c
// Count unique elements
int unique_count = 0;
int prev = -1;  // Use -1 since all products are positive

for (int i = 0; i < total_size; i++) {
    if (i == 0 || all_products[i] != prev) {
        unique_count++;
    }
    prev = all_products[i];
}
```

## Efficiency Analysis

This implementation is efficient for several reasons:

1. **Reduced Computation**: By leveraging the symmetry of the multiplication table, we only compute the upper triangular portion, reducing the work by approximately half.

2. **Load Balancing**: The algorithm distributes work evenly among processes, ensuring each process handles a similar amount of computation.

3. **Memory Efficiency**: Each process only stores its portion of the products, allowing for handling larger values of N.

4. **Minimal Communication**: Processes compute independently and only communicate when gathering results at the end, minimizing communication overhead.

5. **Scalability**: The approach scales well with additional processes, making it suitable for computing M(N) for large values of N (up to 10^5). I reccomend running this on sharcnet (or a similar system) for larger numbers.

## Conclusion

The parallel implementation we have developed, **significantly** outperforms sequential approaches for large values of N. And meets all of the load balancing and scalability requirements of an efficient algorithm.
