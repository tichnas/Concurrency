#define _POSIX_C_SOURCE 199309L  // required for clock
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#define DEBUG 0
#define MINW 2   // min time company takes to prepare vaccines
#define MAXW 5   // max time company takes to prepare vaccines
#define MINR 1   // min no. of batches
#define MAXR 5   // max no. of batches
#define MINP 10  // min capacity of a batch
#define MAXP 20  // max capacity of a batch

typedef struct company {
  int companyNo;
  float probability;
  int batchesLeft;
  int batchCapacity;
  int vaccinesLeft;
} company;

typedef struct zone {
  int zoneNo;
  int capacity;
  int supplier;
  pthread_mutex_t mutex;
} zone;

typedef struct student {
  int studentNo;
  int timesVaccinated;
  int zone;  // 0 = waiting for zone , -1 = not waiting for zone (not arrived or
             // done)
  pthread_mutex_t mutex;
} student;

company companyArr[1000];
zone zoneArr[1000];
student studentArr[1000];

pthread_t companyThread[1000];
pthread_t zoneThread[1000];
pthread_t studentThread[1000];
pthread_mutex_t randomMutex;
pthread_mutex_t slotsMutex;
char timeStr[10] = "00:00";

int studentsLeft;
int slotsAvailable;
int noOfCompanies;
int noOfZones;
int noOfStudents;

void* updateTime() {
  struct timespec ts;
  double startTime;
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  startTime = ts.tv_nsec / (1e9) + ts.tv_sec;

  while (1) {
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    double curTime = ts.tv_nsec / (1e9) + ts.tv_sec;

    int seconds = curTime - startTime;
    int minutes = seconds / 60;
    seconds -= minutes * 60;

    sprintf(timeStr, "\033[0;32m%02d:%02d\033[0m", minutes, seconds);
  }

  return NULL;
}

int min(int a, int b, int c) {
  if (b < a) a = b;
  if (c < a) a = c;
  return a;
}

void* studentJob(void* arg) {
  int studentNo = (*(student*)arg).studentNo;

  pthread_mutex_lock(&randomMutex);
  int arrivalTime = 1 + rand() % 30;
  pthread_mutex_unlock(&randomMutex);

  if (DEBUG) printf("%s Created Student: %d\n", timeStr, studentNo);

  sleep(arrivalTime);

  printf("%s\033[0;31m Student %d has arrived for his round 1 of Vaccination\n",
         timeStr, studentNo);
  printf(
      "%s\033[0;33m Student %d is waiting to be allocated a slot on a "
      "Vaccination Zone\n",
      timeStr, studentNo);
  studentArr[studentNo].zone = 0;

  while (studentArr[studentNo].zone != -1)
    ;

  return NULL;
}

void vaccinateSuccess(int studentNo) {
  printf("%s\033[0;36m Student %d has tested positive for antibodies.\n",
         timeStr, studentNo);
  studentsLeft--;
  studentArr[studentNo].zone = -1;
}

void vaccinateFailure(int studentNo) {
  printf("%s\033[0;36m Student %d has tested negative for antibodies.\n",
         timeStr, studentNo);

  if (studentArr[studentNo].timesVaccinated < 3) {
    printf(
        "%s\033[0;31m Student %d has arrived for his round %d of Vaccination\n",
        timeStr, studentNo, studentArr[studentNo].timesVaccinated + 1);
    printf(
        "%s\033[0;33m Student %d is waiting to be allocated a slot on a "
        "Vaccination Zone\n",
        timeStr, studentNo);
    studentArr[studentNo].zone = 0;
  } else {
    studentsLeft--;
    studentArr[studentNo].zone = -1;
  }
}

void vaccinate(int studentNo) {
  int zoneNo = studentArr[studentNo].zone;
  int companyNo = zoneArr[zoneNo].supplier;
  float probability = companyArr[companyNo].probability;

  companyArr[companyNo].vaccinesLeft--;
  studentArr[studentNo].timesVaccinated++;

  printf(
      "%s\033[0;31m Student %d on Vaccination Zone %d has been vaccinated "
      "which has success probability %.2f\n",
      timeStr, studentNo, zoneNo, probability);

  pthread_mutex_lock(&randomMutex);
  float badLuck = (float)rand() / RAND_MAX;
  pthread_mutex_unlock(&randomMutex);

  if (DEBUG)
    printf("%s Student %d bad luck = %f\n", timeStr, studentNo, badLuck);

  if (badLuck < probability) {
    vaccinateSuccess(studentNo);
  } else {
    vaccinateFailure(studentNo);
  }
}

void* zoneJob(void* arg) {
  int zoneNo = (*(zone*)arg).zoneNo;

  if (DEBUG) printf("%s Created Zone: %d\n", timeStr, zoneNo);

  pthread_mutex_lock(&randomMutex);
  int deliverTime = 1 + rand() % 5;
  pthread_mutex_unlock(&randomMutex);

  sleep(deliverTime);
  printf(
      "%s\033[0;35m Pharmaceutical Company %d has delivered vaccines to "
      "Vaccination zone %d, resuming vaccinations now\n",
      timeStr, zoneArr[zoneNo].supplier, zoneNo);

  while (studentsLeft > 0 && zoneArr[zoneNo].capacity > 0) {
    pthread_mutex_lock(&randomMutex);
    pthread_mutex_lock(&slotsMutex);

    int maxSlots =
        min(8, zoneArr[zoneNo].capacity, studentsLeft - slotsAvailable);
    int k = 0;
    if (maxSlots > 0) k = 1 + rand() % maxSlots;
    slotsAvailable += k;

    pthread_mutex_unlock(&slotsMutex);
    pthread_mutex_unlock(&randomMutex);

    if (k == 0) continue;

    int studentsGot = 0;

    printf(
        "%s\033[0;34m Vaccination Zone %d is ready to vaccinate with %d "
        "slots\n",
        timeStr, zoneNo, k);

    while (studentsLeft > studentsGot && studentsGot < k) {
      for (int studentNo = 1; studentNo <= noOfStudents && studentsGot < k;
           studentNo++) {
        pthread_mutex_lock(&studentArr[studentNo].mutex);

        if (studentArr[studentNo].zone == 0) {
          studentsGot++;
          studentArr[studentNo].zone = zoneNo;
          printf(
              "%s\033[0;35m Student %d assigned a slot on the Vaccination Zone "
              "%d and waiting to be vaccinated\n",
              timeStr, studentNo, zoneNo);
        }

        pthread_mutex_unlock(&studentArr[studentNo].mutex);
      }
    }

    printf("%s\033[0;34m Vaccination Zone %d entering Vaccination Phase\n",
           timeStr, zoneNo);

    sleep(2);

    for (int studentNo = 1; studentNo <= noOfStudents; studentNo++) {
      if (studentArr[studentNo].zone == zoneNo) vaccinate(studentNo);
    }

    pthread_mutex_lock(&slotsMutex);
    slotsAvailable -= k;
    pthread_mutex_unlock(&slotsMutex);
  }

  if (zoneArr[zoneNo].capacity == 0) {
    printf("%s\033[0;33m Vaccination Zone %d has run out of vaccines\n",
           timeStr, zoneNo);
  }

  zoneArr[zoneNo].supplier = -1;
  return NULL;
}

void supply(int companyNo) {
  if (DEBUG) printf("%s Company %d Supplying\n", timeStr, companyNo);

  while (studentsLeft > 0 && companyArr[companyNo].batchesLeft > 0) {
    for (int zoneNo = 1;
         zoneNo <= noOfZones && companyArr[companyNo].batchesLeft > 0;
         zoneNo++) {
      pthread_mutex_lock(&zoneArr[zoneNo].mutex);

      if (zoneArr[zoneNo].supplier == -1) {
        companyArr[companyNo].batchesLeft--;
        zoneArr[zoneNo].supplier = companyNo;
        zoneArr[zoneNo].capacity = companyArr[companyNo].batchCapacity;

        printf(
            "%s\033[0;33m Pharmaceutical Company %d is delivering a vaccine "
            "batch to "
            "Vaccination Zone %d which has success probability %.2f\n",
            timeStr, companyNo, zoneNo, companyArr[companyNo].probability);

        pthread_create(&zoneThread[zoneNo], NULL, zoneJob, &zoneArr[zoneNo]);
      }

      pthread_mutex_unlock(&zoneArr[zoneNo].mutex);
    }
  }
}

void* companyJob(void* arg) {
  int companyNo = (*(company*)arg).companyNo;

  if (DEBUG) printf("%s Created Company: %d\n", timeStr, companyNo);

  while (studentsLeft > 0) {
    pthread_mutex_lock(&randomMutex);
    int w = MINW + rand() % (MAXW - MINW + 1);  // time to prepare vaccines
    int r = MINR + rand() % (MAXR - MINR + 1);  // no. of batches prepared
    int p = MINP + rand() % (MAXP - MINP + 1);  // no. of vaccines in a batch
    pthread_mutex_unlock(&randomMutex);

    printf(
        "%s\033[0;36m Pharmaceutical Company %d is preparing %d batches of "
        "vaccines "
        "which have success probability %.2f\n",
        timeStr, companyNo, r, companyArr[companyNo].probability);
    sleep(w);
    printf(
        "%s\033[0;31m Pharmaceutical Company %d has prepared %d batches of "
        "vaccines "
        "which have success probability %.2f. Waiting for all the vaccines to "
        "be "
        "used to resume production\n",
        timeStr, companyNo, r, companyArr[companyNo].probability);

    companyArr[companyNo].batchCapacity = p;
    companyArr[companyNo].batchesLeft = r;
    companyArr[companyNo].vaccinesLeft = r * p;

    supply(companyNo);

    while (studentsLeft > 0 && companyArr[companyNo].vaccinesLeft > 0)
      ;

    if (companyArr[companyNo].vaccinesLeft == 0) {
      printf(
          "%s\033[0;36m All the vaccines prepared by Pharmaceutical Company %d "
          "are "
          "emptied. Resuming production now.\n",
          timeStr, companyNo);
    }
  }

  return NULL;
}

int main() {
  srand(time(0));
  float probability;

  scanf("%d %d %d", &noOfCompanies, &noOfZones, &noOfStudents);

  if (noOfCompanies > 999 || noOfZones > 999 || noOfStudents > 999) {
    printf("This simulation can't handle too large input.\n");
    return 0;
  }

  studentsLeft = noOfStudents;

  pthread_mutex_init(&randomMutex, NULL);
  pthread_mutex_init(&slotsMutex, NULL);

  for (int studentNo = 1; studentNo <= noOfStudents; studentNo++) {
    studentArr[studentNo].studentNo = studentNo;
    studentArr[studentNo].timesVaccinated = 0;
    studentArr[studentNo].zone = -1;
    pthread_mutex_init(&studentArr[studentNo].mutex, NULL);
  }

  for (int zoneNo = 1; zoneNo <= noOfZones; zoneNo++) {
    zoneArr[zoneNo].zoneNo = zoneNo;
    zoneArr[zoneNo].supplier = -1;
    pthread_mutex_init(&zoneArr[zoneNo].mutex, NULL);
  }

  for (int companyNo = 1; companyNo <= noOfCompanies; companyNo++) {
    scanf("%f", &probability);

    companyArr[companyNo].probability = probability;
    companyArr[companyNo].companyNo = companyNo;
  }

  pthread_t timeTid;
  pthread_create(&timeTid, NULL, updateTime, NULL);

  sprintf(timeStr, "\033[0;32m00:00\033[0m");

  printf("%s\033[0;32m Simulation Start\n", timeStr);

  for (int studentNo = 1; studentNo <= noOfStudents; studentNo++) {
    pthread_create(&studentThread[studentNo], NULL, studentJob,
                   &studentArr[studentNo]);
  }

  for (int companyNo = 1; companyNo <= noOfCompanies; companyNo++) {
    pthread_create(&companyThread[companyNo], NULL, companyJob,
                   &companyArr[companyNo]);
  }

  if (DEBUG) printf("%s Waiting for student threads\n", timeStr);

  for (int studentNo = 1; studentNo <= noOfStudents; studentNo++) {
    pthread_join(studentThread[studentNo], NULL);
  }

  if (DEBUG) printf("%s Waiting for zone threads\n", timeStr);

  for (int zoneNo = 1; zoneNo <= noOfZones; zoneNo++) {
    pthread_join(zoneThread[zoneNo], NULL);
  }

  if (DEBUG) printf("%s Waiting for company threads\n", timeStr);

  for (int companyNo = 1; companyNo <= noOfCompanies; companyNo++) {
    pthread_join(companyThread[companyNo], NULL);
  }

  printf("%s\033[0;32m Simulation Over.\n", timeStr);

  return 0;
}