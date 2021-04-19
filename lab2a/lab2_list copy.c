// NAME: Weikeng Yang
// EMAIL: weikengyang@gmail.com
// ID: 405346443

#include <stdlib.h>
#include <stdio.h> // for fprintf(3)
#include <errno.h> // for errno(3)
#include <getopt.h> // for getopt_long(3)
#include <pthread.h>
#include <time.h> // for clock_gettime(3)
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <string.h>
#include "SortedList.h"

int numThreads = 1; // default 1
int numIterations = 1; // default 1
int numElements;
int opt_yield = 0;
int yield_flag = 0;
char* yieldopts;
char syncopts;
pthread_mutex_t mutex;
int spinLock;
SortedList_t* list; // shared list
SortedListElement_t* elements; // shared list of elements

void segfault_handler() {
  fprintf(stderr, "ERROR: Segmentation fault.\n");
  exit(2);
}

void *thread_routine(void *arg) {
  int num = *((int*) arg);
  int i;
  // insert intialized elements
  for (i = num * numIterations; i < (num+1)*numIterations; i++) {
    if (syncopts == 'm')
      pthread_mutex_lock(&mutex);
      
    else if (syncopts == 's')
      while (__sync_lock_test_and_set(&spinLock,1));
      
    SortedList_insert(list, &elements[i]);

    if (syncopts == 'm')
      pthread_mutex_unlock(&mutex);

    else if (syncopts == 's')
      __sync_lock_release(&spinLock);
  }

  // get the list length
  if (syncopts == 'm')
    pthread_mutex_lock(&mutex);

  else if (syncopts == 's')
    while (__sync_lock_test_and_set(&spinLock,1));

  int len = SortedList_length(list);
  if (len == -1) {
    fprintf(stderr, "List is corrupted when cheking length\n");
    exit(2);
  }

  if (syncopts == 'm')
    pthread_mutex_unlock(&mutex);

  else if (syncopts == 's')
    __sync_lock_release(&spinLock);

  // looks up and delete all the keys it had previously inserted
  for (i = num * numIterations; i < (num+1)*numIterations; i++) {
    if (syncopts == 'm')
      pthread_mutex_lock(&mutex);
      
    else if (syncopts == 's')
      while (__sync_lock_test_and_set(&spinLock,1));
      
    SortedListElement_t *target = SortedList_lookup(list,elements[i].key);
    if (!target) {
      fprintf(stderr, "Failed to lookup element\n");
      exit(2);
    }

    if (SortedList_delete(target) == 1) {
      fprintf(stderr, "Failed to delete target\n");
      exit(2);
    }

    if (syncopts == 'm')
      pthread_mutex_unlock(&mutex);

    else if (syncopts == 's')
      __sync_lock_release(&spinLock);
  }
  return NULL;
  
}

void errorcheck(char* msg) {
  fprintf(stderr, "%s with error: %s\n", msg, strerror(errno));
  exit(1);
}

int main(int argc, char *argv[]) {
  static const struct option long_options[] = {
    {"threads", required_argument, 0, 't'},
    {"iterations", required_argument, 0, 'i'},
    {"yield", required_argument, 0, 'y'},
    {"sync", required_argument, 0 , 's'},
    {0,0,0,0}
  };

  char opt;
  while (1) {
    int option_index = 0;
    opt = getopt_long(argc, argv, "t:i:y:s:", long_options, &option_index);
    
    if (opt == -1)
      break;

    switch (opt) {
      case 't':
        numThreads = atoi(optarg);
        break;
      case 'i':
        numIterations = atoi(optarg);
        break;
      case 'y':
        yield_flag = 1;
        yieldopts = optarg;
        break;
      case 's':
        syncopts = optarg[0];
        break;
      default:
        fprintf(stderr, "ERROR: Unrecognized argument. Correct usage: lab2_add '--threads=#' '--iterations=#' '--yield=[idl]' '--sync=[s][m]\n");
        exit(1);
    }
  }

  signal(SIGSEGV, segfault_handler);

  if (yield_flag) {
    int i;
    int len_yieldopts = strlen(yieldopts);
    for (i = 0; i < len_yieldopts; i++) {
      if (yieldopts[i] == 'i')
        opt_yield |= INSERT_YIELD;
      else if (yieldopts[i] == 'd')
        opt_yield |= DELETE_YIELD;
      else if (yieldopts[i] == 'l')
        opt_yield |= LOOKUP_YIELD;
    }
  }

  // initializes mutex
  if (syncopts == 'm') {
    if (pthread_mutex_init(&mutex, NULL) != 0)
      errorcheck("Failed to initialize mutex");
  }

  // initializes spin lock
  if (syncopts == 's')
    spinLock = 0;

  // initialize empty list
  list = (SortedList_t*) malloc(sizeof(SortedList_t));
  if (!list)
    errorcheck("Failed to allocate memory to initialize list");
  list->key = NULL;
  list->prev = list;
  list->next = list;

  // creates and initializes the required number of list elements
  // Sources: https://stackoverflow.com/questions/3591226/generate-random-keys
  numElements = numThreads * numIterations;
  elements = (SortedListElement_t*) malloc(numElements * sizeof(SortedListElement_t));
  if (!elements)
    errorcheck("Failed to allocate memory to initialize list elements");

  char a[] = "abcdefghijklmnopqrstuvwxyz";
  int i;
  for (i = 0; i < numElements; i++) {
    char random[2];
    random[0] = a[rand()%26];
    random[1] = '\0';
    elements[i].key = random;
  }

  // allocate memory for threads id
  pthread_t* tid = malloc(numThreads * sizeof(pthread_t));
  if (!tid)
    errorcheck("Failed to allocate memory for threads ids");

  // notes the (high resolution) starting time for the run
  struct timespec startTime, endTime;
  clock_gettime(CLOCK_MONOTONIC, &startTime);

  // create threads
  int nums[numThreads];
  for (i = 0; i < numThreads; i++) {
      nums[i] = i;
      if (pthread_create(&tid[i], NULL, &thread_routine, &nums[i]) != 0)
        errorcheck("Failed to create threads");
  }

  for (i = 0; i < numThreads; i++) {
    if (pthread_join(tid[i], NULL) != 0)
      errorcheck("Failed to join threads");
  }

  clock_gettime(CLOCK_MONOTONIC, &endTime);

  // checks the length of the list to confirm that it's zero
  if (SortedList_length(list) != 0) {
    fprintf(stderr, "Length of list is not zero\n");
    exit(2);
  }

  // print out .csv values
  int numOps = numThreads * numIterations * 3;
  int totalRuntime = (endTime.tv_sec - startTime.tv_sec) * 1000000000L + (endTime.tv_nsec - startTime.tv_nsec);
  int avgPerOp = totalRuntime / numOps;
  int numLists = 1;

  fprintf(stdout, "list-");
  if (opt_yield) {
    if (opt_yield & INSERT_YIELD)
      fprintf(stdout, "i");
    if (opt_yield & DELETE_YIELD)
      fprintf(stdout, "d");
    if (opt_yield & LOOKUP_YIELD)
      fprintf(stdout, "l");
  }

  else
    fprintf(stdout, "none");

  switch (syncopts) {
    case 0:
      fprintf(stdout, "-none");
      break;
    case 's':
      fprintf(stdout, "-s");
      break;
    case 'm':
      fprintf(stdout, "-m");
      break;
    default:
      break;
  }

  fprintf(stdout, ",%d,%d,%d,%d,%d,%d\n", numThreads, numIterations, numLists, numOps, totalRuntime, avgPerOp);

  free(elements);
  free(list);
  free(tid);
  return 0;
}
