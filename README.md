# Multiplication Table Analysis

A parallel implementation for computing the number of unique elements in N×N multiplication tables using the Message Passing Interface. Began as a group assignment but I've continued it for practice and optimization.

## Results

When run on a Ryzen 9 5900X (12 cores, 24 threads) with 24 parallel processes, the program produced these results:

| N     | Total Products | Unique Elements (M(N)) | % Unique | Runtime (seconds) |
|-------|----------------|------------------------|----------|-------------------|
| 10    | 100            | 42                     | 42.00%   | 0.002264          |
| 50    | 2,500          | 800                    | 32.00%   | 0.002393          |
| 100   | 10,000         | 2,906                  | 29.06%   | 0.002782          |
| 500   | 250,000        | 64,673                 | 25.87%   | 0.019317          |
| 1000  | 1,000,000      | 248,083                | 24.81%   | 0.072280          |
| 2000  | 4,000,000      | 959,759                | 23.99%   | 0.282714          |
| 5000  | 25,000,000     | 5,770,205              | 23.08%   | 1.774261          |
| 10000 | 100,000,000    | 22,504,348             | 22.50%   | 7.358289          |
| 20000 | 400,000,000    | 87,938,320             | 21.98%   | 30.703627         |
| 30000 | 900,000,000    | 195,378,691            | 21.71%   | 69.146723         |
| 40000 | 1,600,000,000  | 344,462,009            | 21.53%   | 127.397301        |
| 50000 | 2,500,000,000  | 534,772,334            | 21.39%   | 379.415311        |

Key finding: Only about 21% of numbers in large multiplication tables are unique. I'll need to test for larger values of N to see if there is a pattern limiting around 21%.

## Problem Statement

A multiplication table shows products of numbers. For example, a 3×3 table looks like:
```
    1  2  3
  ┌─────────
1 │ 1  2  3
2 │ 2  4  6
3 │ 3  6  9
```

This table has 9 spots but only 6 different values (1, 2, 3, 4, 6, 9).

The goal is to find M(N) - how many different unique values exist in an N×N table.

## How the Program Works

The code uses several efficiency techniques:

1. Only computes half the table since it's symmetric
2. Splits work across multiple CPU cores
3. Uses a hash table to track unique numbers
4. Includes a 64-bit version for huge tables (N > 40,000)

### The Hash Table

```c
typedef struct {
    int* buckets;
    int size;
    int count;
} HashSet;

// Add a value to the hash set
bool hashset_add(HashSet* set, int value) {
    unsigned int pos = hash(value, set->size);
    while (set->buckets[pos] != -1) {
        if (set->buckets[pos] == value) {
            return false;  // Already exists
        }
        pos = (pos + 1) % set->size;  // Linear probing
    }
    
    // Insert value
    set->buckets[pos] = value;
    set->count++;
    return true;
}
```

### Computing the Products

```c
// Compute products and track unique ones
while (global_idx <= end_idx && i <= N) {
    int product = (int)(i * j);
    hashset_add(&unique_products, product);
    
    global_idx++;
    j++;
    if (j > N) {
        i++;
        j = i;
    }
}
```

## How to Run It

### Windows (with MS-MPI)

1. Install MS-MPI from [Microsoft's MS-MPI page](https://www.microsoft.com/en-us/download/details.aspx?id=57467)

2. Compile the program:
   ```
   gcc multiplication.c -o multiplication.exe -I"C:\Program Files (x86)\Microsoft SDKs\MPI\Include" -L"C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64" -lmsmpi
   ```

3. Run with a specific N value:
   ```
   "C:\Program Files\Microsoft MPI\Bin\mpiexec.exe" -n 24 multiplication.exe 10000
   ```
   
   For very large tables, use the 64-bit version:
   ```
   "C:\Program Files\Microsoft MPI\Bin\mpiexec.exe" -n 24 multiplication_opt_64bit.exe 50000
   ```

## Key Findings

1. Only about 21% of the products in big multiplication tables are unique.

2. Using multiple CPU cores makes the program much faster for big tables.

3. The 64-bit version can handle extremely large tables (N=50,000).

4. The percentage of unique values approaches 21% as tables get bigger, suggesting an interesting mathematical pattern.
