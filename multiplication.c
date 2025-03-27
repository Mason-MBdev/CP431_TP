#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// binary search function to determine if a number is unique or not

bool is_unique(int array[], int size, int target) {
    int left = 0;
    int right = size - 1;
    while (left <= right) {
        int middle = left + (right - left) / 2;
        if (array[middle] == target) return false;
        if (array[middle] < target) left = middle + 1;
        else right = middle - 1;
    }
    return true;
}


// main function

int main(int argc, char *argv[]) {
  MPI_Init(&argc, argv);
  int world_size, world_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &size);
  MPI_Comm_size(MPI_COMM_WORLD, &rank);

  // main process (rank = 0) prompts for value N
  // and broadcasts this to all processes

  int N;
  if (world_rank == 0) {
    printf("Enter N: ");
    scanf("%d", &N);
  }
  MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

  // set first and last item to define range for each process

  int first = ( N / world_size ) * rank + 1;
  int last = (rank == world_size - 1) ? N : start + (N / size) - 1; 
  
  // local storage of all unique products found by a process

  int *products = ((int *)malloc(N * N * sizeof(int))); 

  // local count for the number of unique elements in a process

  int count = 0;

  // loops through portioned range and computes products of i * j
  // if the product is unique, increases the count and adds it to the products array

  for (int i = first; i <= last; i++) {
    for (int j = 1; j <= N; j++) {
        int product = i * j;
        if (is_unique(productss, count, product)) {
            values[count++] = product;
        }
    }
  }

// left to do:
//  1. merge process
//  2. compute global results
//  3. free memory
//  4. maybe other stuff I forgot

}



  

  
