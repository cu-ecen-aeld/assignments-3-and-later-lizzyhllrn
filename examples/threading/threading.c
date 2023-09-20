#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    
    //printf("in threadfunc\n");
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    
    thread_func_args->thread_complete_success = false;
    usleep(thread_func_args->wait_time);
    
    int rc = pthread_mutex_lock(thread_func_args->p_mutex);
    //printf("locked mutex? rc is %d\n", rc);
    if (rc != 0) {
      printf("pthread_mutex_lock failed with %d\n", rc);
    } else {
      //printf("pthread_mutex_lock success?\n");
      thread_func_args->thread_complete_success = true;
    
    
    usleep(thread_func_args->release_time);
    rc = pthread_mutex_unlock(thread_func_args->p_mutex);
    if (rc != 0) {
      printf("pthread_mutex_unlock failed with %d\n", rc);
      thread_func_args->thread_complete_success = false;
    } 
    }
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
     printf("in start obtaining mutex\n");
     struct thread_data* thread_param = (struct thread_data *) malloc(sizeof(struct thread_data));
     thread_param->p_mutex = mutex;
     thread_param->wait_time = wait_to_obtain_ms;
     thread_param->release_time = wait_to_release_ms;
     thread_param->thread_complete_success=true;
     
     int rc = pthread_create(thread, NULL, &threadfunc, thread_param);
     if (rc != 0) {
      printf("pthread_create failed with %d\n", rc);
     }
    return thread_param->thread_complete_success;
}

