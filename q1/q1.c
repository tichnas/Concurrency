#define _POSIX_C_SOURCE 199309L  // required for clock
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>

void printArr(int n, int* arr) {
  for (int i = 0; i < n; i++) printf("%d ", arr[i]);
  printf("\n");
}

int isSorted(int n, int* arr) {
  for (int i = 0; i + 1 < n; i++) {
    if (arr[i] > arr[i + 1]) return 0;
  }

  return 1;
}

struct arg {
  int start;
  int end;
  int* arr;
};

int* shareMem(size_t size) {
  key_t mem_key = IPC_PRIVATE;
  int shm_id = shmget(mem_key, size, IPC_CREAT | 0666);
  return (int*)shmat(shm_id, NULL, 0);
}

void selectionSort(int* arr, int start, int end) {
  for (int i = start; i <= end; i++) {
    int minIndex = i;

    for (int j = i + 1; j <= end; j++) {
      if (arr[j] < arr[minIndex]) minIndex = j;
    }

    int temp = arr[i];
    arr[i] = arr[minIndex];
    arr[minIndex] = temp;
  }
}

void merge(int* arr, int start, int end) {
  int mid = (start + end) / 2;
  int lSize = mid - start + 1;
  int rSize = end - mid;
  int leftArr[lSize];
  int rightArr[rSize];
  int lp = 0;
  int rp = 0;

  for (int i = 0; i < lSize; i++) leftArr[i] = arr[i + start];
  for (int i = 0; i < rSize; i++) rightArr[i] = arr[i + mid + 1];

  int i = start;

  while (lp < lSize && rp < rSize) {
    if (leftArr[lp] < rightArr[rp]) {
      arr[i++] = leftArr[lp++];
    } else {
      arr[i++] = rightArr[rp++];
    }
  }

  while (lp < lSize) arr[i++] = leftArr[lp++];
  while (rp < rSize) arr[i++] = rightArr[rp++];
}

void mergeSort(int* arr, int start, int end) {
  // fprintf(stderr, "Start: %d %d\n", start, end);
  if (end - start < 4) {
    // fprintf(stderr, "Short: %d %d\n", start, end);
    selectionSort(arr, start, end);

    return;
  }

  int mid = (start + end) / 2;
  mergeSort(arr, start, mid);
  mergeSort(arr, mid + 1, end);
  merge(arr, start, end);
}

void mergeSortProcess(int* arr, int start, int end) {
  // fprintf(stderr, "Starting: %d %d\n", start, end);
  if (end - start < 4) {
    // fprintf(stderr, "Short: %d %d\n", start, end);
    selectionSort(arr, start, end);

    return;
  }

  int mid = (start + end) / 2;

  int pidLeft = fork();

  if (pidLeft < 0) {
    perror("Fork Error");
  } else if (!pidLeft) {
    mergeSortProcess(arr, start, mid);
    exit(0);
  } else {
    int pidRight = fork();

    if (pidRight < 0) {
      perror("Fork Error");
    } else if (!pidRight) {
      mergeSortProcess(arr, mid + 1, end);
      exit(0);
    } else {
      waitpid(pidLeft, NULL, 0);
      waitpid(pidRight, NULL, 0);
      // fprintf(stderr, "Done: %d %d\n", start, end);

      merge(arr, start, end);
    }
  }
}

void* mergeSortThread(void* a) {
  struct arg* args = (struct arg*)a;
  int* arr = args->arr;
  int start = args->start;
  int end = args->end;

  if (end - start < 4) {
    // fprintf(stderr, "Short: %d %d\n", start, end);
    selectionSort(arr, start, end);

    return NULL;
  }

  int mid = (start + end) / 2;

  struct arg argsLeft;
  argsLeft.start = start;
  argsLeft.end = mid;
  argsLeft.arr = arr;

  struct arg argsRight;
  argsRight.start = mid + 1;
  argsRight.end = end;
  argsRight.arr = arr;

  pthread_t tidLeft;
  if (pthread_create(&tidLeft, NULL, mergeSortThread, &argsLeft))
    perror("Thread Error");

  pthread_t tidRight;
  if (pthread_create(&tidRight, NULL, mergeSortThread, &argsRight))
    perror("Thread Error");

  pthread_join(tidLeft, NULL);
  pthread_join(tidRight, NULL);

  merge(arr, start, end);

  return NULL;
}

void initializeThreadSort(int* arr, int start, int end) {
  struct arg args;
  args.start = start;
  args.end = end;
  args.arr = arr;

  pthread_t tid;
  if (pthread_create(&tid, NULL, mergeSortThread, &args))
    perror("Thread Error");
  pthread_join(tid, NULL);
}

double getTime(int* arr, int n, void (*fun)(int*, int, int)) {
  struct timespec ts;
  double startTime, endTime;

  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  startTime = ts.tv_nsec / (1e9) + ts.tv_sec;

  fun(arr, 0, n - 1);

  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  endTime = ts.tv_nsec / (1e9) + ts.tv_sec;

  return endTime - startTime;
}

void reset(int n, int* orig, int* arr) {
  for (int i = 0; i < n; i++) {
    arr[i] = orig[i];
  }
}

void run(int* arr, int n, double* times, int normal) {
  int origArr[n];
  reset(n, arr, origArr);

  if (normal) printf("\nRunning normal Merge Sort...\n");

  times[0] = getTime(arr, n, mergeSort);

  if (!normal && !isSorted(n, arr)) {
    printf("ERROR -Normal!!!\n");
    printArr(n, origArr);
    printArr(n, arr);
  }

  if (normal) printArr(n, arr);

  reset(n, origArr, arr);

  if (normal) printf("\nRunning concurrent Merge Sort using processes...\n");

  times[1] = getTime(arr, n, mergeSortProcess);

  if (!normal && !isSorted(n, arr)) {
    printf("ERROR -Process!!!\n");
    printArr(n, origArr);
  }

  if (normal) printArr(n, arr);

  reset(n, origArr, arr);

  if (normal) printf("\nRunning threaded Merge Sort...\n");

  times[2] = getTime(arr, n, initializeThreadSort);

  if (!normal && !isSorted(n, arr)) {
    printf("ERROR -Thread!!!\n");
    printArr(n, origArr);
  }

  if (normal) printArr(n, arr);

  reset(n, origArr, arr);
}

void randomizer(int min, int max, double* times, int* arr) {
  int timesRun = 10;

  int done = 0;
  times[0] = times[1] = times[2] = 0;

  while (done++ < timesRun) {
    int n = min + (rand() % (max - min + 1));
    double tempTimes[3];

    printf("\nRunning random input %d with n = %d\n", done, n);

    for (int i = 0; i < n; i++) arr[i] = rand() % (100001);

    // printf("%d\n", n);
    // for (int i = 0; i < n; i++) printf("%d ", arr[i]);

    run(arr, n, tempTimes, 0);

    for (int i = 0; i < 3; i++) times[i] += tempTimes[i];
  }

  for (int i = 0; i < 3; i++) times[i] /= timesRun;
}

void normalStart() {
  srand(time(0));
  int n;
  double times[3];
  int* arr = shareMem(sizeof(int) * 100000);

  // randomizer(10000, 10000, times, arr);

  // printf("Enter n (no. of elements): ");
  scanf("%d", &n);

  // printf("Enter %d numbers\n", n);
  for (int i = 0; i < n; i++) scanf("%d", arr + i);

  run(arr, n, times, 1);

  printf(
      "\n\033[0;36mNormal Merge Sort: \033[0;32m%lfs"
      "\n\033[0;36mMerge Sort using Processes: \033[0;32m%lfs"
      "\n\033[0;36mMerge Sort using Threads: \033[0;32m%lfs"
      "\n\033[0;35mNormal Merge sort is %.2f times faster than concurrent "
      "(using process)"
      "\n\033[0;35mNormal Merge sort is %.2f times faster than concurrent "
      "(using threads)\n",
      times[0], times[1], times[2], times[1] / times[0], times[2] / times[0]);
}

void powerStart() {
  srand(time(0));

  int n, choice;
  double times[3];
  int* arr = shareMem(sizeof(int) * 100000);

  printf("\033[1;32mMerge Sort Analysis\033[0m\n");

  while (1) {
    do {
      printf(
          "\n\033[0;31m0. \033[0;34mExit\n\033[0;31m"
          "1. \033[0;34mCustom Input\n\033[0;31m"
          "2. \033[0;34mRandomizer (Takes average of 10 random inputs)\n"
          "Enter choice no.: \033[0;31m");
      scanf("%d", &choice);

      printf("\033[0m");

      if (!choice) return;

      if (choice == 1 || choice == 2) {
        break;
      } else {
        printf("Invalid Choice !!!\n");
      }
    } while (1);

    if (choice == 1) {
      printf("\nEnter n (no. of elements): ");
      scanf("%d", &n);

      printf("Enter %d numbers\n", n);
      for (int i = 0; i < n; i++) scanf("%d", arr + i);

      printf("\033[0;33m");
      run(arr, n, times, 1);
      printf("\033[0m");
    } else {
      do {
        printf(
            "\n\033[0;31m0. \033[0;34mExit\n\033[0;31m"
            "1. \033[0;34mSmall n (10 to 100)\n\033[0;31m"
            "2. \033[0;34mMedium n (100 to 1000)\n\033[0;31m"
            "3. \033[0;34mLarge n (1000 to 10000)\n"
            "Enter choice no.: \033[0;31m");
        scanf("%d", &choice);

        printf("\033[0m");

        if (!choice) return;

        if (choice == 1 || choice == 2 || choice == 3) {
          break;
        } else {
          printf("Invalid Choice !!!\n");
        }
      } while (1);

      printf("\033[0;33m");

      if (choice == 1) {
        randomizer(10, 100, times, arr);

      } else if (choice == 2) {
        randomizer(100, 1000, times, arr);

      } else {
        randomizer(1000, 10000, times, arr);
      }

      printf("\033[0m");
    }

    printf(
        "\n\033[0;36mNormal Merge Sort: \033[0;32m%lfs"
        "\n\033[0;36mMerge Sort using Processes: \033[0;32m%lfs"
        "\n\033[0;36mMerge Sort using Threads: \033[0;32m%lfs"
        "\n\033[0;35mNormal Merge sort is %.2f times faster than concurrent "
        "(using process)"
        "\n\033[0;35mNormal Merge sort is %.2f times faster than concurrent "
        "(using threads)\n",
        times[0], times[1], times[2], times[1] / times[0], times[2] / times[0]);
  }
}

int main(int argc, char** argv) {
  if (argc == 2 && argv[1][0] == '1' && argv[1][1] == '\0') {
    powerStart();
  } else {
    normalStart();
  }
  return 0;
}