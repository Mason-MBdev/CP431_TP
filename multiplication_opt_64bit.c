#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

// Hash table implementation for efficient unique element tracking
#define HASH_SIZE 16777259 // Large prime number for hash table size
#define LOAD_FACTOR_THRESHOLD 0.7

typedef struct {
    int64_t* buckets;
    int64_t size;
    int64_t count;
} HashSet;

// Initialize a hash set
void hashset_init(HashSet* set, int64_t size) {
    set->size = size;
    set->count = 0;
    set->buckets = (int64_t*)calloc(size, sizeof(int64_t));
    // Initialize all buckets to -1 (empty)
    for (int64_t i = 0; i < size; i++) {
        set->buckets[i] = -1;
    }
}

// Hash function
uint64_t hash(int64_t value, int64_t size) {
    uint64_t h = (uint64_t)value;
    h = ((h >> 32) ^ h) * 0x45d9f3b;
    h = ((h >> 32) ^ h) * 0x45d9f3b;
    h = (h >> 32) ^ h;
    return h % size;
}

// Add a value to the hash set, returns true if added (was not present)
bool hashset_add(HashSet* set, int64_t value) {
    // Skip if value is 0 (our marker for deleted/empty)
    if (value <= 0) return false;

    // Check load factor and resize if needed
    if ((double)set->count / set->size > LOAD_FACTOR_THRESHOLD) {
        // Create a new larger hash set
        HashSet new_set;
        int64_t new_size = set->size * 2;
        hashset_init(&new_set, new_size);
        
        // Rehash all existing elements
        for (int64_t i = 0; i < set->size; i++) {
            if (set->buckets[i] > 0) {
                hashset_add(&new_set, set->buckets[i]);
            }
        }
        
        // Free old buckets and update set
        free(set->buckets);
        set->buckets = new_set.buckets;
        set->size = new_size;
        set->count = new_set.count;
    }
    
    // Find position using linear probing
    uint64_t pos = hash(value, set->size);
    while (set->buckets[pos] != -1) {
        // If already exists, return false
        if (set->buckets[pos] == value) {
            return false;
        }
        // Linear probing
        pos = (pos + 1) % set->size;
    }
    
    // Insert value
    set->buckets[pos] = value;
    set->count++;
    return true;
}

// Free the hash set
void hashset_free(HashSet* set) {
    free(set->buckets);
    set->buckets = NULL;
    set->size = 0;
    set->count = 0;
}

// Convert hash set to array for MPI transfer
int64_t* hashset_to_array(HashSet* set, int64_t* size) {
    int64_t* array = (int64_t*)malloc(set->count * sizeof(int64_t));
    int64_t idx = 0;
    
    for (int64_t i = 0; i < set->size; i++) {
        if (set->buckets[i] > 0) {
            array[idx++] = set->buckets[i];
        }
    }
    
    *size = idx;
    return array;
}

// Merge two sorted arrays into a new sorted array, counting unique elements
int64_t merge_unique_count(int64_t* arr1, int64_t size1, int64_t* arr2, int64_t size2) {
    if (size1 == 0) return size2;
    if (size2 == 0) return size1;
    
    int64_t i = 0, j = 0;
    int64_t unique_count = 0;
    int64_t last = -1;
    
    while (i < size1 && j < size2) {
        if (arr1[i] < arr2[j]) {
            if (arr1[i] != last) {
                unique_count++;
                last = arr1[i];
            }
            i++;
        } else if (arr1[i] > arr2[j]) {
            if (arr2[j] != last) {
                unique_count++;
                last = arr2[j];
            }
            j++;
        } else { // Equal elements
            if (arr1[i] != last) {
                unique_count++;
                last = arr1[i];
            }
            i++;
            j++;
        }
    }
    
    // Handle remaining elements
    while (i < size1) {
        if (arr1[i] != last) {
            unique_count++;
            last = arr1[i];
        }
        i++;
    }
    
    while (j < size2) {
        if (arr2[j] != last) {
            unique_count++;
            last = arr2[j];
        }
        j++;
    }
    
    return unique_count;
}

// In-place merge sort implementation for int64_t
void merge(int64_t arr[], int64_t left, int64_t mid, int64_t right) {
    int64_t i, j, k;
    int64_t n1 = mid - left + 1;
    int64_t n2 = right - mid;
    
    // Create temp arrays
    int64_t* L = (int64_t*)malloc(n1 * sizeof(int64_t));
    int64_t* R = (int64_t*)malloc(n2 * sizeof(int64_t));
    
    // Copy data to temp arrays
    for (i = 0; i < n1; i++)
        L[i] = arr[left + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[mid + 1 + j];
    
    // Merge the temp arrays back
    i = 0;
    j = 0;
    k = left;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        } else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }
    
    // Copy remaining elements
    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }
    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }
    
    free(L);
    free(R);
}

// Merge sort function
void merge_sort(int64_t arr[], int64_t left, int64_t right) {
    if (left < right) {
        int64_t mid = left + (right - left) / 2;
        
        // Sort first and second halves
        merge_sort(arr, left, mid);
        merge_sort(arr, mid + 1, right);
        
        // Merge the sorted halves
        merge(arr, left, mid, right);
    }
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    double start_time, end_time;
    
    if (world_rank == 0) {
        start_time = MPI_Wtime();
    }

    // Get N from command line argument, default to 10 for safety
    int64_t N = 10;
    if (argc > 1) {
        N = atoll(argv[1]);
        
        // Verify N is within reasonable bounds
        if (N <= 0) {
            if (world_rank == 0) {
                printf("Error: N must be positive\n");
                fflush(stdout);
            }
            MPI_Finalize();
            return 1;
        }
    } else if (world_rank == 0) {
        printf("No value provided for N, using default N=10\n");
        fflush(stdout);
    }
    
    // Broadcast N to all processes
    MPI_Bcast(&N, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);

    // Calculate total number of products in upper triangular matrix
    int64_t total_pairs = ((int64_t)N * ((int64_t)N + 1)) / 2;
    int64_t pairs_per_proc = total_pairs / world_size;
    int64_t remainder = total_pairs % world_size;
    
    // Determine start and end indexes
    int64_t start_idx = world_rank * pairs_per_proc + (world_rank < remainder ? world_rank : remainder);
    int64_t end_idx = start_idx + pairs_per_proc + (world_rank < remainder ? 1 : 0) - 1;
    
    if (world_rank == 0) {
        printf("Computing M(%" PRId64 ") with %d processes...\n", N, world_size);
        fflush(stdout);
    }

    // Initialize hash set for this process
    // Choose initial size based on expected number of unique elements
    int64_t initial_hashset_size = (end_idx - start_idx + 1) / 4;
    if (initial_hashset_size < 1024) initial_hashset_size = 1024;
    
    HashSet unique_products;
    hashset_init(&unique_products, initial_hashset_size);
    
    // Compute all products for assigned portion using a more efficient approach
    int64_t i = 1;
    int64_t j = i;
    int64_t global_idx = 0;
    
    // Fast-forward to starting position
    while (global_idx < start_idx) {
        global_idx++;
        j++;
        if (j > N) {
            i++;
            j = i;
        }
    }
    
    // Compute products for our range and add directly to hash set
    while (global_idx <= end_idx && i <= N) {
        int64_t product = i * j;
        hashset_add(&unique_products, product);
        
        global_idx++;
        j++;
        if (j > N) {
            i++;
            j = i;
        }
    }

    // Signal completion of local computation
    int local_done = 1;
    int all_done = 0;
    MPI_Allreduce(&local_done, &all_done, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    
    if (world_rank == 0 && all_done == world_size) {
        printf("All processes have computed their unique products\n");
        fflush(stdout);
    }

    // Convert local unique products to array for MPI transfer
    int64_t local_unique_count = unique_products.count;
    int64_t* local_unique_products = hashset_to_array(&unique_products, &local_unique_count);
    
    // Sort the local array for easier merging
    merge_sort(local_unique_products, 0, local_unique_count - 1);
    
    // Need to use int for MPI calls since MPI doesn't handle int64_t directly
    int local_count_int = (int)local_unique_count;
    int* all_counts = NULL;
    if (world_rank == 0) {
        all_counts = (int*)malloc(world_size * sizeof(int));
    }
    
    MPI_Gather(&local_count_int, 1, MPI_INT, all_counts, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    // Process 0 gathers all unique products and merges them
    int64_t global_unique_count = 0;
    
    if (world_rank == 0) {
        // Calculate displacements for MPI_Gatherv
        int total_products_int = 0;
        int* displacements = (int*)malloc(world_size * sizeof(int));
        
        for (int i = 0; i < world_size; i++) {
            displacements[i] = total_products_int;
            total_products_int += all_counts[i];
        }
        
        // Allocate memory for all gathered products (as int64_t)
        int64_t total_products = (int64_t)total_products_int;
        int64_t* all_products = (int64_t*)malloc(total_products * sizeof(int64_t));
        
        // Gather all local unique products
        // We need to cast to MPI_LONG_LONG for 64-bit integers
        MPI_Gatherv(local_unique_products, local_count_int, MPI_LONG_LONG, 
                   all_products, all_counts, displacements, 
                   MPI_LONG_LONG, 0, MPI_COMM_WORLD);
        
        // Sort all gathered products
        merge_sort(all_products, 0, total_products - 1);
        
        // Count unique elements in the merged array
        if (total_products > 0) {
            global_unique_count = 1;  // First element is always unique
            for (int64_t i = 1; i < total_products; i++) {
                if (all_products[i] != all_products[i-1]) {
                    global_unique_count++;
                }
            }
        }
        
        end_time = MPI_Wtime();
        
        // Print results with total products for comparison
        // Use format specifiers for int64_t
        printf("M(%" PRId64 ") = %" PRId64 "\n", N, global_unique_count);
        printf("Total products in table: %" PRId64 "\n", (int64_t)N * (int64_t)N);
        printf("Percentage of unique products: %.2f%%\n", 
               (double)global_unique_count / ((double)N * (double)N) * 100.0);
        printf("Time elapsed: %.6f seconds\n", end_time - start_time);
        fflush(stdout);
        
        // Free memory
        free(all_products);
        free(displacements);
        free(all_counts);
    } else {
        // Non-root processes just send their data
        MPI_Gatherv(local_unique_products, local_count_int, MPI_LONG_LONG, 
                   NULL, NULL, NULL, 
                   MPI_LONG_LONG, 0, MPI_COMM_WORLD);
    }
    
    // Clean up
    free(local_unique_products);
    hashset_free(&unique_products);
    
    MPI_Finalize();
    return 0;
} 