#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

pthread_t t_threadfn;     

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;

	struct thread_data* thread_func_args = (struct thread_data *) thread_param;

	DEBUG_LOG("Started threadfunc with parameters, obtain = %d\t release = %d\t", thread_func_args->wait_to_obtain_ms, thread_func_args->wait_to_release_ms);
	/* Check if invalid time value */
	if(thread_func_args->wait_to_obtain_ms < 0)
		usleep(0);

	/* Wait to obtain sleep */
	usleep((thread_func_args->wait_to_obtain_ms)*1000);

	/* Lock the mutex */
	int rc = pthread_mutex_lock(thread_func_args->mutex);
	if(rc != 0)
	{
		ERROR_LOG("pthread_mutex_lock failed");
		return thread_param;
	}

	/* Check if invalid time value */
	if(thread_func_args->wait_to_release_ms < 0)
		usleep(0);

	/* Wait to release sleep */
	usleep((thread_func_args->wait_to_release_ms)*1000);

	/* Unlock the mutex */
	rc = pthread_mutex_unlock(thread_func_args->mutex);
	if(rc != 0)
	{
		ERROR_LOG("pthread_mutex_unlock failed");
		return thread_param;
	}

	/* Return thread complete status */
	thread_func_args->thread_complete_success = true;

	DEBUG_LOG("Thread complete");
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
     	struct thread_data *t_data = malloc(sizeof(struct thread_data));
   
	/* Filling up thread data struct with inputs */
	t_data->wait_to_obtain_ms  = wait_to_obtain_ms;
	t_data->wait_to_release_ms = wait_to_release_ms;
	t_data->mutex	       = mutex; 
	t_data->thread_complete_success = false;

	DEBUG_LOG("Completed initialization");

	/* Creating the mutex thread to test */
	int rc = pthread_create(&t_threadfn, NULL, &threadfunc, t_data);    
	
	/* Error occurred in pthread create */
	if(rc != 0)
		return false;

	/* Send the thread ID back to the caller */
	*thread = t_threadfn;
	return true;

}

