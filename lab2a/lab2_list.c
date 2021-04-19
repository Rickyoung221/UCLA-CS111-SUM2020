// NAME: Weikeng Yang
// EMAIL: weikengyang@gmail.com
// ID: 405346443

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>
#include "SortedList.h"

SortedList_t* list;
SortedListElement_t* elements;

int num_threads;
int num_iterations;
int num_elements;
int opt_yield;
int yield_flag = 0;
char* protected;
char opt_sync;
pthread_mutex_t m_lock;
int spinLock;


void handle_segfault() {
    fprintf(stderr, "ERROR! Segmentation fault.\n");
    exit(2);
}

char *generate_key(){
    char* key = (char*) malloc(2*sizeof(char));
    key[0] = (char) (rand() % 26) + 'a';
    key[1] = '\0';
    return key;
}

void *thread_funct(void *arg) {
    int num = *((int*) arg);
    int i;

    for (i = num * num_iterations; i < (num+1)*num_iterations; i++) {
        if (opt_sync == 'm'){
            pthread_mutex_lock(&m_lock);
        } else if (opt_sync == 's'){
            while (__sync_lock_test_and_set(&spinLock, 1));
        }
      
        SortedList_insert(list, &elements[i]);

        if (opt_sync == 'm'){   //Unlock the mutex
            pthread_mutex_unlock(&m_lock);
        } else if (opt_sync == 's'){  //Release the spin lock
            __sync_lock_release(&spinLock);
        }
    }

    // Get the length of the list
    //Lock
    if (opt_sync == 'm'){
        pthread_mutex_lock(&m_lock);
    }
    else if (opt_sync == 's'){
        while (__sync_lock_test_and_set(&spinLock,1));
    }
    
    int list_length = SortedList_length(list);
    if (list_length == -1) {
        fprintf(stderr, "ERROR! Cannot get the list length.\n");
        exit(2);
    }
    if (list_length < num_iterations) {
        fprintf(stderr, "EROOR! Some items did not insert in the list.\n");
        exit(2);
    }
    //Unlock
    if (opt_sync == 'm'){
        pthread_mutex_unlock(&m_lock);
    }else if (opt_sync == 's'){
        __sync_lock_release(&spinLock);
    }

    for (i = num * num_iterations; i < (num+1)*num_iterations; i++) {
        if (opt_sync == 'm'){
            pthread_mutex_lock(&m_lock);
        }else if (opt_sync == 's'){
            while (__sync_lock_test_and_set(&spinLock,1));
        }
      
      SortedListElement_t *search = SortedList_lookup(list, elements[i].key);
      if (search == NULL) {
          fprintf(stderr, "ERROR! Cannot lookup the element.\n");
          exit(2);
      }

      if (SortedList_delete(search) == 1) {
          fprintf(stderr, "ERROR! Cannot delete the target.\n");
          exit(2);
      }
      
      
      if (opt_sync == 'm'){   //Unlock the mutex
          pthread_mutex_unlock(&m_lock);
      } else if (opt_sync == 's'){  //Release the spin lock
          __sync_lock_release(&spinLock);
      }
  }
    return NULL;
  
}



int main(int argc, char **argv) {
    int opt;
    
    num_iterations = 1; // default 1
    opt_yield = 0;
    num_threads = 1; // default 1
    
    static const struct option long_options[] = {
        {"threads", required_argument, NULL, 't'},
        {"iterations", required_argument, NULL, 'i'},
        {"yield", required_argument, NULL, 'y'},
        {"sync", required_argument, NULL , 's'},
        {0,0,0,0}
    };


    while (1) {
        opt = getopt_long(argc, argv, "t:i:y:s:", long_options, NULL);
    
        if (opt == -1)
            break;

        switch (opt) {
            case 't':
                num_threads = atoi(optarg);
                break;
            case 'i':
                num_iterations = atoi(optarg);
                break;
            case 'y':
                yield_flag = 1;
                protected = optarg;
                break;
            case 's':
                opt_sync = optarg[0];
                break;
            default:
                fprintf(stderr, "Unrecognized argument\n");
                fprintf(stderr, "--threads=#, default 1\n");
                fprintf(stderr, "--iterations=#, default 1\n");
                fprintf(stderr, "--yield=[idl], default no yield\n");
                exit(1);
    }
  }

    signal(SIGSEGV, handle_segfault);

    if (yield_flag) {
        int i;
        int length = strlen(protected);
        for (i = 0; i < length; i++) {
            switch (protected[i]) {
              case 'i':
                opt_yield |= INSERT_YIELD;
                break;
              case 'd':
                opt_yield |= DELETE_YIELD;
                break;
              case 'l':
                opt_yield |= LOOKUP_YIELD;
                break;
              default:
                fprintf(stderr, "Invalid yield argument.\n");
                exit(1);
            }
        }
    }


    if (opt_sync == 'm') {
        if (pthread_mutex_init(&m_lock, NULL) != 0){
            fprintf(stderr, "ERROR！ Cannot initialize mutex.\n");
            exit(1);
        }
    }

    if (opt_sync == 's')
        spinLock = 0;

    list = (SortedList_t*) malloc(sizeof(SortedList_t));
    if (list == NULL){
        fprintf(stderr, "ERROR! Cannot allocate memory.\n");
        exit(1);
    }
    list->key = NULL;
    list->prev = list;
    list->next = list;


    num_elements = num_threads * num_iterations;
    elements = (SortedListElement_t*) malloc(num_elements * sizeof(SortedListElement_t));
    if (elements == NULL){
        fprintf(stderr, "ERROR! Cannot allocate memory to initialize the list elements.\n");
        exit(1);
    }


    int i;
    for (i = 0; i < num_elements; i++) {
        elements[i].key = generate_key();
    }

  // allocate memory for threads id
    pthread_t* threads_id = malloc(num_threads * sizeof(pthread_t));
    if (!threads_id){
        fprintf(stderr, "ERROR! Cannot allocate memory for threads ids.\n");
        exit(1);
        
    }
     

  // notes the (high resolution) starting time for the run
    struct timespec start, end;
    //get time
    if(clock_gettime(CLOCK_REALTIME, &start) == -1) {
        fprintf(stderr, "ERROR! Failed to get time.\n");
        exit(1);
    }


  // create threads
    int nums[num_threads];
    for (i = 0; i < num_threads; i++) {
        nums[i] = i;
        if (pthread_create(&threads_id[i], NULL, &thread_funct, &nums[i]) != 0){
            fprintf(stderr, "ERROR! Cannot create threads.\n");
            exit(1);
        }
    }

    for (i = 0; i < num_threads; i++) {
        if (pthread_join(threads_id[i], NULL) != 0){
            fprintf(stderr, "ERROR! Cannot join threads.\n");
            exit(1);
        }
    }

    if(clock_gettime(CLOCK_REALTIME, &end) == -1) {
        fprintf(stderr, "ERROR! Failed to get time.\n");
        exit(1);
    }
    
    /* finish get time */
    
    

  // checks the length of the list to confirm that it's zero
    if (SortedList_length(list) != 0) {
        fprintf(stderr, "Length of list is not 0. \n");
        exit(2);
    }

  fprintf(stdout, "list-");
    
  if (opt_yield) {
      if (opt_yield & INSERT_YIELD)
          fprintf(stdout, "i");
      if (opt_yield & DELETE_YIELD)
          fprintf(stdout, "d");
      if (opt_yield & LOOKUP_YIELD)
          fprintf(stdout, "l");
    }else{
        fprintf(stdout, "none");
    }
    
    
    
    switch (opt_sync) {
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
    
    

    int num_operations = num_threads * num_iterations * 3;
    int runtime = (1000000000L * (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec));
    int time_per_operation =  runtime / num_operations;
    int num_lists = 1;
    
    
    //print data
    fprintf(stdout, ",%d,%d,%d,%d,%d,%d\n", num_threads, num_iterations, num_lists, num_operations, runtime, time_per_operation);

    free(elements);
    free(threads_id);
    free(list);

    if (opt_sync == 'm') {
        pthread_mutex_destroy(&m_lock);
    }
    exit(0);
}
