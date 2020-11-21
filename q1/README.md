# Concurrent Merge Sort

## Input

1. First line of input contains _n_ (size of array)
2. Next _n_ numbers are array elements

```
10
75 16 70 57 75 12 13 83 40 31
```

> Optional: Run menu-based program with even more options by giving _1_ as an argument when running program

## Function & Structure Descriptions

- `arg`

  - Used to pass arguments in threaded merge sort
  - Stores the start, end and the array itself

- `printArr`

  - Takes array and its size as parameter
  - Prints the given array

- `shareMem`

  - Takes size as parameter
  - Returns pointer to shared memory

- `selectionSort`

  - Takes start, end and array as parameter
  - Performs selection sort on the array from start to end

- `merge`

  - Takes start, end and array as parameter
  - Merges the first and second half of the array and stores back in the array

- `mergeSort`

  - Takes start, end and array as parameter
  - Perform merge sort by recursively calling itself

- `mergeSortProcess`

  - Takes start, end and array as parameter
  - Same as `mergeSort`, but create different processes for each half

- `mergeSortThread`

  - Takes start, end and array as parameter in form of `arg` structure
  - Same as `mergeSort`, but create different threads for each half

- `initializeThreadSort`

  - Takes start, end and array as parameter
  - Creates thread to start threaded merge sort

- `getTime`

  - Takes array, its size and a function to run
  - Returns the time to run the function

- `reset`

  - Takes two arrays, _orig_ and _arr_ and their size as parameter
  - Reset arr to orig

- `run`

  - Takes array, its size, an array _times_ and a variable _normal_
  - Runs all types of merge sort on the given array
  - Assigns time taken for each to the _times_ array
  - If _normal_ is 1, then print the sorted array and some other statements

- `normalStart`

  - Starter function

- `powerStart`
  - Function to access more options in the program
  - Menu driven
  - Option to randomize input and get average in different range of array size
  - Uses function `randomizer` to generate random input
  - Runs `isSorted` function everytime to make sure the array is sorted

## Code Flow (Logic)

1. `normalStart` takes the input, runs `run` and then print the result
2. `run` takes the input array and an array to store different times
3. Reset the array everytime and send different functions to `getTime`
4. `getTime` takes the function, run it and return the time taken
5. Different functions passed to `getTime` are `mergeSort`, `mergeSortProcess` and `initializeThreadSort`
6. `initializeThreadSort` just creates a different thread to run `mergeSortThread`
7. All `mergeSort`, `mergeSortProcess` and `mergeSortThread` are nearly same with the only difference being the second one creates different processes and the third one creates different threads
8. They call themselves recursively to sort the two halves
9. Merge the two sorted halves using `merge`
10. If the length of the sub-array is less than 5, then run `selectionSort`

## Findings & Conclusion

I ran the code 100 times with random input for different ranges of _n_ (size of array) and the result after averaging them are given below.

> The results may vary depending on the system, free RAM and CPU usage on the time of executing.

### 10 <= n <= 100

- Timings

  - Normal Merge Sort: 0.000008 seconds
  - Merge Sort using Processes: 0.005394 seconds
  - Merge Sort using Threads: 0.001048 seconds

- Comparison

  - Normal Merge sort is 710.09 times faster than concurrent (using process)
  - Normal Merge sort is 138.00 times faster than concurrent (using threads)

### 100 <= n <= 1000

- Timings

  - Normal Merge Sort: 0.000081 seconds
  - Merge Sort using Processes: 0.102983 seconds
  - Merge Sort using Threads: 0.008256 seconds

- Comparison

  - Normal Merge sort is 1278.27 times faster than concurrent (using process)
  - Normal Merge sort is 102.48 times faster than concurrent (using threads)

### 1000 <= n <= 10000

- Timings

  - Normal Merge Sort: 0.000887 seconds
  - Merge Sort using Processes: 1.640541 seconds
  - Merge Sort using Threads: 0.074471 seconds

- Comparison

  - Normal Merge sort is 1848.66 times faster than concurrent (using process)
  - Normal Merge sort is 83.92 times faster than concurrent (using threads)

### Possible Explanation for the findings

- Normal Merge sort is faster than concurrent because:

  - Continuously creating & destroying threads/processes requires a lot of time
  - Context Switching also requires time
  - The processor loads the left half in cache when executing it, but just after a few instructions it switches to the right half. This causes a lot of cache misses and slows down the program

- Threaded Merge sort is faster than Merge sort using processes because:

  - Creating/Destroying a thread is much faster than creating/destroying a process
  - Context switching is also much faster in case of threads
