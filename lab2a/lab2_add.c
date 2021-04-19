// NAME: Weikeng Yang
// EMAIL: weikengyang@gmail.com
// ID: 405346443


/* This is test driver program */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sched.h>

int opt_yield;
int num_threads;
int num_iterations;

char* protected;
long long counter;

pthread_mutex_t m_lock = PTHREAD_MUTEX_INITIALIZER;
int spinLock = 0;


//Provided from class material
void add(long long *pointer, long long value) {
    long long sum = *pointer + value;
    if (opt_yield){
        sched_yield();
    }
    *pointer = sum;
}

void add_mutex(long long *pointer, long long value) {
    pthread_mutex_lock(&m_lock);
    long long sum = *pointer + value;
    if (opt_yield){
    sched_yield();
    }
    *pointer = sum;
     pthread_mutex_unlock(&m_lock);
}

void thread_add() {
    int i;
    for (i = 0; i < num_iterations; i++) {
        add(&counter, 1);
    }
    for (i = 0; i < num_iterations; i++) {
        add(&counter, -1);
    }
}


void addSpin(long long *pointer, long long value) {
    while(__sync_lock_test_and_set(&spinLock, 1));
    long long sum = *pointer + value;
    if (opt_yield){
        sched_yield();
    }
    *pointer = sum;
    __sync_lock_release(&spinLock);
}

void threadAddSpin() {
    int i;
    for (i = 0; i < num_iterations; i++) {
        addSpin(&counter, 1);
    }
    for (i = 0; i < num_iterations; i++) {
        addSpin(&counter, -1);
    }
}

void add_c(long long *pointer, long long value) {
    long long old;
    do {
        old = counter;

        if (opt_yield){
            sched_yield();
        }
    } while (__sync_val_compare_and_swap(pointer, old, old + value) != old);
}

void threadAddAtomic() {
    int i;
    for (i = 0; i < num_iterations; i++) {
        add_c(&counter, 1);
        add_c(&counter, -1);
    }
}

void threadAddMutex() {
    int i;
    for (i = 0; i < num_iterations; i++) {
        add_mutex(&counter, 1);
    }
    for (i = 0; i < num_iterations; i++) {
        add_mutex(&counter, -1);
    }
}


int main(int argc, char **argv) {

    num_threads = 1;
    num_iterations = 1;
    opt_yield = 0;
    protected = "a";
    
    static struct option long_options[] = {
        {"threads",  required_argument,  NULL,  't'},
        {"iterations",  required_argument,  NULL,  'i'},
        {"yield",  optional_argument,  NULL,  'y'},
        {"sync",  required_argument,  NULL,  's'},
        {0, 0, 0, 0}
    };

    while(1) {
        int opt = getopt_long(argc, argv, "t:i:ys:", long_options, NULL);
        if (opt == -1)
            break;
        switch(opt) {
            case 't':
                num_threads = atoi(optarg);
                break;
            case 'i':
                num_iterations = atoi(optarg);
                break;
            case 'y':
                opt_yield = 1;
                break;
            case 's':
                protected = optarg;
                if (strncmp(protected, "m", 1) != 0 && strncmp(protected, "s", 1) != 0 && strncmp(protected, "c", 1) != 0) {
                    fprintf(stderr, "ERROR! Invalid synchronization. \n");
                }
                break;
            default:
                fprintf(stderr, "ERROR! Unrecognized argument. \n");
                exit(1);
      }
  }
    //Run
    struct timespec start, end;
    void (*addfunc) = &thread_add;
    if (strncmp(protected, "m", 1) == 0)
        addfunc = &thread_aAddMutex;
    else if (strncmp(protected, "s", 1) == 0)
        addfunc = &threadAddSpin;
    else if (strncmp(protected, "c", 1) == 0)
        addfunc = &threadAddAtomic;

    pthread_t *threads_id = malloc(sizeof(pthread_t)*num_threads);
    if (threads_id == NULL) {
        fprintf(stderr, "ERROR! Cannot allocat memory for threads.\n");
        exit(1);
    }


    //create threads to add 1 and -1 to counter
    int i;
    for (i = 0; i < num_threads; i++) {
        if(pthread_create(&threads_id[i], NULL, (void *) addfunc, NULL) != 0) {
            fprintf(stderr, "ERROR! Cannot create threads.\n");
            exit(1);
        }
    }
    
    for (i = 0; i < num_threads; i++) {
        pthread_join(threads_id[i], NULL);
    }
    
    //get time
    if(clock_gettime(CLOCK_REALTIME, &start) == -1 || clock_gettime(CLOCK_REALTIME, &end) == -1) {
        fprintf(stderr, "ERROR! Failed to get time.\n");
        exit(1);
    }

    long runtime = (1000000000 * (end.tv_sec - start.tv_sec)) + (end.tv_nsec - start.tv_nsec);
    int operations = num_threads * num_iterations*2;
    long time_per_operation =  runtime / operations;
    
    //Print

    char tag[2048] = "";
    char* yield = "";
    char* sync = "-none";
    
    if (opt_yield)
      yield = "-yield";

    if (strncmp(protected, "m", 1) == 0)
        sync = "-m";
    else if (strncmp(protected, "s", 1) == 0)
        sync = "-s";
    else if (strncmp(protected, "c", 1) == 0)
        sync = "-c";

    strcat(tag, "add");
    strcat(tag, yield);
    strcat(tag, sync);

    printf("%s,%d,%d,%d,%ld,%ld,%lld\n", tag, num_threads, num_iterations, operations, runtime, time_per_operation, counter);

    free(threads_id);
    exit(0);
}
