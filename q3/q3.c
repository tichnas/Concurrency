#define _POSIX_C_SOURCE 199309L  // required for clock
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#define DEBUG 0

typedef struct {
  int id;
  pthread_mutex_t mutex;
  pthread_cond_t condition;
  char instrument;
  int arrivalTime;
  char name[20];
  int stageId;
  int status;  // 0 = Waiting to perform, 1 = Performing, 2 = Left
} Performer;

typedef struct {
  int id;
  pthread_mutex_t mutex;
  char type;
  int musician;
  int singer;
} Stage;

typedef struct {
  Performer* performer;
  int acoustic;
  int electric;
} PerformerCarrier;

char timeStr[10] = "00:00";

Performer performerArr[1000];
Stage stageArr[1000];

int maxWaitingTime;
int minPerformanceTime;
int maxPerformanceTime;
int noOfStages;
int noOfPerformers;
int noOfCoordinators;

sem_t electricSemaphore;
sem_t acousticSemaphore;
sem_t singerSemaphore;
sem_t coordinatorSemaphore;
sem_t exitedSemaphore;
pthread_mutex_t randomMutex;

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

int random(int min, int max) {
  pthread_mutex_lock(&randomMutex);
  int num = min + rand() % (max - min + 1);
  pthread_mutex_unlock(&randomMutex);
  return num;
}

void* maxWait(void* arg) {
  Performer* performer = (Performer*)arg;

  sleep(maxWaitingTime);

  pthread_mutex_lock(&(performer->mutex));

  if (performer->status == 0) {
    printf("%s\033[0;31m %s %c left because of impatience\n", timeStr,
           performer->name, performer->instrument);
    if (DEBUG)
      printf("%s %s released by impatience\n", timeStr, performer->name);
    sem_post(&exitedSemaphore);
    performer->status = 2;
  }

  pthread_mutex_unlock(&(performer->mutex));

  return NULL;
}

void soloPerform(Performer* performer) {
  int performanceDuration = random(minPerformanceTime, maxPerformanceTime);
  int singerId;

  printf("%s\033[0;35m %s performing %c at %s stage (no. %d) for %d sec\n",
         timeStr, performer->name, performer->instrument,
         stageArr[performer->stageId].type == 'a' ? "acoustic" : "electric",
         performer->stageId, performanceDuration);
  pthread_mutex_unlock(&(performer->mutex));

  sleep(performanceDuration);

  int extendDuration = 0;

  if (performer->instrument != 's') {
    pthread_mutex_lock(&stageArr[performer->stageId].mutex);
    singerId = stageArr[performer->stageId].singer;
    if (singerId) extendDuration = 2;
    pthread_mutex_unlock(&stageArr[performer->stageId].mutex);
    if (DEBUG)
      printf("%s %d sec extended for %s\n", timeStr, extendDuration,
             performer->name);
  }

  sleep(extendDuration);

  pthread_mutex_lock(&stageArr[performer->stageId].mutex);

  if (extendDuration) {
    printf("%s\033[0;33m %s performance at %s stage finished\n", timeStr,
           performerArr[singerId].name,
           stageArr[performer->stageId].type == 'a' ? "acoustic" : "electric");

    pthread_cond_signal(&performerArr[singerId].condition);
  }

  printf("%s\033[0;33m %s performance at %s stage finished\n", timeStr,
         performer->name,
         stageArr[performer->stageId].type == 'a' ? "acoustic" : "electric");
  stageArr[performer->stageId].musician = 0;
  stageArr[performer->stageId].singer = 0;

  pthread_mutex_unlock(&stageArr[performer->stageId].mutex);
}

void jointPerform(Performer* performer) {
  printf(
      "%s\033[0;36m %s joined %s's performance, performance extended by 2 "
      "secs\n",
      timeStr, performer->name,
      performerArr[stageArr[performer->stageId].musician].name);

  pthread_cond_wait(&(performer->condition), &(performer->mutex));
  pthread_mutex_unlock(&(performer->mutex));

  if (DEBUG)
    printf("%s %s done with conditional wait\n", timeStr, performer->name);
}

void getTShirt(Performer* performer) {
  sem_wait(&coordinatorSemaphore);

  printf("%s\033[0;34m %s collecting T-Shirt\n", timeStr, performer->name);
  sleep(2);
  printf("%s\033[0;34m %s got the T-Shirt\n", timeStr, performer->name);

  sem_post(&coordinatorSemaphore);
}

void* handlePerformer(void* arg) {
  PerformerCarrier* carrier = (PerformerCarrier*)arg;
  Performer* performer = carrier->performer;
  int getAcoustic = carrier->acoustic;
  int getElectric = carrier->electric;
  int hasAcoustic = 0;
  int hasElectric = 0;
  int soloAcoustic = 0;
  int soloElectric = 0;
  int isSinger = (getAcoustic & getElectric);
  int performed = 0;

  while (performer->status == 0) {
    if (isSinger) {
      sem_wait(&singerSemaphore);
      hasAcoustic = (sem_trywait(&acousticSemaphore) == 0 ? 1 : 0);
      hasElectric = (sem_trywait(&electricSemaphore) == 0 ? 1 : 0);
    } else if (getAcoustic) {
      sem_wait(&acousticSemaphore);
      hasAcoustic = 1;
    } else if (getElectric) {
      sem_wait(&electricSemaphore);
      hasElectric = 1;
    }

    pthread_mutex_lock(&(performer->mutex));
    if (performer->status != 0) {
      if (hasElectric) sem_post(&electricSemaphore);
      if (hasAcoustic) sem_post(&acousticSemaphore);
      if (isSinger) sem_post(&singerSemaphore);
      free(carrier);
      return NULL;
    }

    if (DEBUG) printf("%s %s looking for stage\n", timeStr, performer->name);

    for (int i = 1; i <= noOfStages; i++) {
      if (isSinger || (stageArr[i].type == 'a' && hasAcoustic) ||
          (stageArr[i].type == 'e' && hasElectric)) {
        pthread_mutex_lock(&stageArr[i].mutex);

        if (stageArr[i].singer == 0 &&
            (stageArr[i].musician == 0 || isSinger)) {
          performer->stageId = i;
          performer->status = 1;

          if (isSinger) {
            stageArr[i].singer = performer->id;
          } else {
            stageArr[i].musician = performer->id;
          }

          if (stageArr[i].singer == 0 || stageArr[i].musician == 0) {
            if (stageArr[i].type == 'a') soloAcoustic = 1;
            if (stageArr[i].type == 'e') soloElectric = 1;
          }

          pthread_mutex_unlock(&stageArr[i].mutex);
          break;
        }
        pthread_mutex_unlock(&stageArr[i].mutex);
      }
    }

    if (hasAcoustic && !soloAcoustic) sem_post(&acousticSemaphore);
    if (hasElectric && !soloElectric) sem_post(&electricSemaphore);

    if (performer->status == 0) {
      pthread_mutex_unlock(&(performer->mutex));
    } else {
      if (stageArr[performer->stageId].musician == 0 ||
          stageArr[performer->stageId].singer == 0) {
        soloPerform(performer);
      } else {
        jointPerform(performer);
      }
      performed = 1;
    }

    if (isSinger) sem_post(&singerSemaphore);
    if (soloAcoustic) sem_post(&acousticSemaphore);
    if (soloElectric) sem_post(&electricSemaphore);
  }

  if (performed) {
    getTShirt(performer);

    if (DEBUG)
      printf("%s %s released by completing\n", timeStr, performer->name);
    sem_post(&exitedSemaphore);

    free(carrier);
  }
  return NULL;
}

void* performerJob(void* arg) {
  Performer* performer = (Performer*)arg;
  char instrument = performer->instrument;

  sleep(performer->arrivalTime);
  printf("%s\033[0;31m %s %c arrived\n", timeStr, performer->name,
         performer->instrument);
  performer->status = 0;

  pthread_t waitingThread;
  pthread_create(&waitingThread, NULL, maxWait, performer);

  if (instrument == 's') {
    pthread_t tid;
    PerformerCarrier* carrier = malloc(sizeof(PerformerCarrier));
    carrier->performer = performer;
    carrier->acoustic = 1;
    carrier->electric = 1;
    pthread_create(&tid, NULL, handlePerformer, carrier);
  }
  if (instrument == 'p' || instrument == 'g' || instrument == 'v') {
    pthread_t tid;
    PerformerCarrier* carrier = malloc(sizeof(PerformerCarrier));
    carrier->performer = performer;
    carrier->acoustic = 1;
    carrier->electric = 0;
    pthread_create(&tid, NULL, handlePerformer, carrier);
  }
  if (instrument == 'p' || instrument == 'g' || instrument == 'b') {
    pthread_t tid;
    PerformerCarrier* carrier = malloc(sizeof(PerformerCarrier));
    carrier->performer = performer;
    carrier->acoustic = 0;
    carrier->electric = 1;
    pthread_create(&tid, NULL, handlePerformer, carrier);
  }

  return NULL;
}

int main() {
  srand(time(0));
  pthread_t timeTid;
  pthread_t performerThread[1000];

  int noOfAcousticStages;
  int noOfElectricStages;
  scanf("%d %d %d %d %d %d %d", &noOfPerformers, &noOfAcousticStages,
        &noOfElectricStages, &noOfCoordinators, &minPerformanceTime,
        &maxPerformanceTime, &maxWaitingTime);

  for (int i = 1; i <= noOfPerformers; i++) {
    scanf("%s %c %d", performerArr[i].name, &performerArr[i].instrument,
          &performerArr[i].arrivalTime);
  }

  noOfStages = noOfAcousticStages + noOfElectricStages;

  pthread_mutex_init(&randomMutex, NULL);
  sem_init(&acousticSemaphore, 0, noOfAcousticStages);
  sem_init(&electricSemaphore, 0, noOfElectricStages);
  sem_init(&singerSemaphore, 0, noOfStages);
  sem_init(&coordinatorSemaphore, 0, noOfCoordinators);
  sem_init(&exitedSemaphore, 0, 0);

  if (noOfCoordinators <= 0) {
    printf("No coordinators to distribute T-Shirts\n");
    return 0;
  }

  pthread_create(&timeTid, NULL, updateTime, NULL);
  sprintf(timeStr, "\033[0;32m00:00\033[0m");
  printf("%s\033[0;32m Simulation Started\n", timeStr);

  for (int i = 1; i <= noOfStages; i++) {
    stageArr[i].id = i;
    stageArr[i].type = (i <= noOfAcousticStages ? 'a' : 'e');
    pthread_mutex_init(&stageArr[i].mutex, NULL);
  }

  for (int i = 1; i <= noOfPerformers; i++) {
    performerArr[i].id = i;
    pthread_mutex_init(&performerArr[i].mutex, NULL);

    pthread_create(&performerThread[i], NULL, performerJob, &performerArr[i]);
  }

  for (int i = 1; i <= noOfPerformers; i++) sem_wait(&exitedSemaphore);

  printf("%s\033[0;32m Simulation Finished\n", timeStr);

  return 0;
}