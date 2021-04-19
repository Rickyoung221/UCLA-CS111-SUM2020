// NAME: Weikeng Yang
// EMAIL: weikengyang@gmail.com
// ID: 405346443

#include "SortedList.h"
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <limits.h>

int num_threads = 1; //# of threads, default 1
int num_iterations = 1; // number of iterations, default 1
int num_lists = 1; // number of lists, default 1
int num_elements = 0; // number of elements
int opt_yield = 0; //yield options
char opt_sync;
char** keys; //used for deletion
SortedListElement_t* elements;
pthread_mutex_t *mutex_locks = NULL;
pthread_t* thread_id = NULL;  //threads
const int NANO = 1000000000;

typedef struct SubList {
    SortedList_t* head;
    pthread_mutex_t mutex;
    int spin_lock;
} SubList_t;

SubList_t* list;  // array of head nodes

typedef struct lock_stat {
    long long nsec;
    int n;
} LockStat_t;
int opt_threads, opt_iters;

unsigned int hash_function(const char* key) {
    int ret = 0;
    while (*key) {
        ret += *key;
        key++;
    }
    return ret % num_lists;
}

void handle_segfault() {
    fprintf(stderr, "ERROR! Segmentation fault! Exiting......\n");
    exit(2);
}

void error_msg(char* msg) {
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

void freeall(pthread_t *thread_id){
    int i;
    for (i = 0; i < num_elements; ++i){
        free((void*)elements[i].key);
    }
    for (i = 0; i < num_lists; i++){
        free(list[i].head);
    }
    free(mutex_locks);
    free(list);
    free(thread_id);
    free(elements);
}
void time_recording(SubList_t* current_list, long long lock_nsec, int num_locks){
    switch(opt_sync)
    {
        struct timespec start;
        struct timespec finish;
        case 'm':
            if (clock_gettime(CLOCK_MONOTONIC, &start) == -1){
                error_msg("ERROR! Cannot get time. Existing....\n");
            }
            pthread_mutex_lock(&current_list->mutex);
            if (clock_gettime(CLOCK_MONOTONIC, &finish) == -1){
                error_msg("ERROR! Cannot get time. Existing....\n");
            }
            lock_nsec += NANO * (finish.tv_sec - start.tv_sec) + (finish.tv_nsec - start.tv_nsec);
            num_locks += 1;
            break;
        case 's':
            if (clock_gettime(CLOCK_MONOTONIC, &start) == -1){
                error_msg("ERROR! Cannot get time. Existing....\n");
            }
            while (__sync_lock_test_and_set(&current_list->spin_lock,1));
            if (clock_gettime(CLOCK_MONOTONIC, &finish) == -1){
                error_msg("ERROR! Cannot get time. Existing....\n");
            }
            lock_nsec += NANO * (finish.tv_sec - start.tv_sec) + (finish.tv_nsec - start.tv_nsec);
            num_locks += 1;
            break;
        default:
            break;
    }
}
void helper(SubList_t* current_list){
    if (opt_sync == 'm'){
        pthread_mutex_unlock(&current_list->mutex);
    }else if (opt_sync == 's'){
        __sync_lock_release(&current_list->spin_lock);
    }
}
//Print the result
void printer(long long run_time, int num_operations, long long avg_time_per_operation, long long waiting_time){
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
    fprintf(stdout, ",%d,%d,%d,%d,%lld,%lld,%lld\n", num_threads, num_iterations, num_lists, num_operations, run_time, avg_time_per_operation, waiting_time);
    
}

void *thread_funct(void *arg) {
    int num = *((int*) arg);
    int i, j;
    SubList_t* current_list;
    struct timespec start;
    struct timespec finish;
    int num_locks = 0;
    long long lock_nsec = 0;
    // 1
    for (i = num * num_iterations; i < (num+1)*num_iterations; i++) {
        j = 0;
        if (num_lists != 1){
            j = hash_function(elements[i].key);
        }
        current_list = &list[j];
        time_recording(current_list, lock_nsec, num_locks);
        SortedList_insert(current_list->head, &elements[i]);
        helper(current_list);
    }

  // 2.  get length of all heads for the total list lenght
    int list_length = 0;
    int sum;

    for (i = 0; i < num_lists; i++) {
        current_list = &list[i];
        time_recording(current_list, lock_nsec, num_locks);
        list_length = SortedList_length(current_list->head);
        if (list_length == -1) {
            fprintf(stderr, "ERROR! The list cannot be read. \n");
            exit(2);
        }
        helper(current_list);
        sum += list_length;
    }

    //3
    for (i = num * num_iterations; i < (num+1)*num_iterations; i++) {
        j = 0;
        if (num_lists != 1){
            j = hash_function(elements[i].key);
        }
        current_list = &list[j];
        //Here cannot be done by using the helper function (would have error), but could work if isolate this switch statement. Didn't know why
        switch(opt_sync)
        {
            case 'm':
                if (clock_gettime(CLOCK_MONOTONIC, &start) == -1){
                    error_msg("ERROR! Cannot get time. Existing....\n");
                }
                pthread_mutex_lock(&current_list->mutex);
                if (clock_gettime(CLOCK_MONOTONIC, &finish) == -1){
                    error_msg("ERROR! Cannot get time. Existing....\n");
                }
                lock_nsec += NANO * (finish.tv_sec - start.tv_sec) + (finish.tv_nsec - start.tv_nsec);
                num_locks += 1;
                break;
            case 's':
                if (clock_gettime(CLOCK_MONOTONIC, &start) == -1){
                    error_msg("ERROR! Cannot get time. Existing....\n");
                }
                while (__sync_lock_test_and_set(&current_list->spin_lock,1));
                if (clock_gettime(CLOCK_MONOTONIC, &finish) == -1){
                    error_msg("ERROR! Cannot get time. Existing....\n");
                }
                lock_nsec += NANO * (finish.tv_sec - start.tv_sec) + (finish.tv_nsec - start.tv_nsec);
                num_locks += 1;
                break;
            default:
                break;
        }
        SortedListElement_t *target = SortedList_lookup(current_list->head,elements[i].key);
        if (!target) {
                fprintf(stderr, "ERROR! Cannot lookup element.\n");
                exit(2);
            }
        if (SortedList_delete(target) == 1) {
                fprintf(stderr, "ERROR! Cannot delete target.\n");
                exit(2);
            }
        helper(current_list);
  }
    LockStat_t* temp = (LockStat_t*) malloc(sizeof(LockStat_t));
    temp -> nsec = lock_nsec;
    temp -> n = num_locks;
    return temp;
}

int main(int argc, char* argv[])
{
    //Deal with segfault
    signal(SIGSEGV, handle_segfault);
    int opt;
    static const struct option long_options[] = {
        {"threads", required_argument, NULL, 't'},
        {"iterations", required_argument, NULL, 'i'},
        {"yield", required_argument, NULL, 'y'},
        {"sync", required_argument,  NULL , 's'},
        {"lists", required_argument, NULL, 'l'},
        {0,0,0,0}
    };

    while (1) {
        opt = getopt_long(argc, argv, "t:i:y:s:l:", long_options, NULL);
    
        if (opt == -1)
            break;
        unsigned long i;
        switch (opt) {
            case 't':
                num_threads = atoi(optarg);
                break;
            case 'i':
                num_iterations = atoi(optarg);
                break;
            case 'y':
                for (i = 0; i < strlen(optarg); i++) {
                    if (optarg[i] == 'i') {
                        opt_yield |= INSERT_YIELD;
                    } else if (optarg[i] == 'd') {
                        opt_yield |= DELETE_YIELD;
                    } else if (optarg[i] == 'l') {
                        opt_yield |= LOOKUP_YIELD;
                    }
                }
                break;
            case 's':
                opt_sync = optarg[0];
                break;
            case 'l':
                num_lists = atoi(optarg);
                if (num_lists < 1)
                {
                    fprintf(stderr, "ERROR! The number of lists should be greater than 0. Exiting...\n");
                    exit(1);
                }
                break;
            default:
                fprintf(stderr, "Unrecognized argument\n");
                fprintf(stderr, "--threads=#, default 1\n");
                fprintf(stderr, "--iterations=#, default 1\n");
                fprintf(stderr, "--yield=[idl], default no yield\n");
                exit(1);
            }
    }

    
    list = (SubList_t*) malloc(num_lists * sizeof(SubList_t));
    if (list == NULL){
        error_msg("ERROR! Cannot allocate memory for list head.\n");
    }

    int i;
    
    for (i = 0; i < num_lists; i++) {
        SubList_t* sublist = &list[i];
        sublist->head = (SortedList_t*) malloc(sizeof(SortedList_t));
        sublist->head->key = NULL;
        sublist->head->prev = sublist -> head;
        sublist->head->next = sublist -> head;

        // Initialize the spin-lock & mutex
        if (opt_sync == 's'){
            sublist->spin_lock = 0;
        }else if (opt_sync == 'm') {
            if (pthread_mutex_init(&sublist->mutex, NULL) != 0)
                error_msg("ERROR! Cannot create mutex. \n");
        }
    }
    
    num_elements = num_threads * num_iterations;
    //allocate all elements
    elements = (SortedListElement_t*) malloc(sizeof(SortedListElement_t) * num_elements);
    keys = (char**) malloc(sizeof(char*) * num_elements);

    for (i = 0; i < num_elements; i++) {
        keys[i] = (char*) malloc(sizeof(char) * 256);
        int j;
        for (j = 0; j < 256; j++) {
            keys[i][j] = rand() % 94 + 33;
        }
        elements[i].key = keys[i];
    }

    // Allocate memory for thread_id
    thread_id = (pthread_t*) malloc(num_threads * sizeof(pthread_t));
    if (thread_id == NULL){
        error_msg("ERROR! Failed to allocate memory for threads! Existing....");
    }

    //Start record time
    struct timespec start;
    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0){
        error_msg("ERROR! Failed to get time. Existing....");
    }

    int nums[num_threads];
    for (i = 0; i < num_threads; i++) {
        nums[i] = i;
        if (pthread_create(&thread_id[i], NULL, &thread_funct, &nums[i]) != 0){
            error_msg("ERROR! Failed to create threads. Existing...");
        }
    }

    long long total_lock_time = 0;
    int total_num_locks = 0;
    LockStat_t* ret;
    for (i = 0; i < num_threads; i++) {
        if (pthread_join(thread_id[i], (void**)&ret) != 0){
            error_msg("ERROR! Failed to join threads. Existing....");
        }
        total_lock_time += ret-> nsec;
        total_num_locks += ret-> n;
        free(ret);
    }
    //Finish, take time
    struct timespec finish;
    if (clock_gettime(CLOCK_MONOTONIC, &finish) < 0){
        error_msg("ERROR! Failed to get time. Existing....");
    }

    int length;
    for (i = 0; i < num_lists; i++) {
        length = SortedList_length(list[i].head);
        if (length != 0) {
            fprintf(stderr, "ERROR! Length of list is not zero. \n");
            exit(2);
        }
    }
    
    int num_operations = num_threads * num_iterations * 3;
    int convert_to_nano = 1000000000L;
    long long run_time = (convert_to_nano * (finish.tv_sec - start.tv_sec))+ (finish.tv_nsec - start.tv_nsec);
    long long time_per_operation = run_time / num_operations;
    long long waiting_time = 0;
    waiting_time = total_lock_time / total_num_locks;

    //Print the result
    printer(run_time, num_operations, time_per_operation, waiting_time);
    //Free the memory
    freeall(thread_id);
    exit(0);
}
