/*
******************************************* 
CSE 438 Assignment -1
Author:- Roshan Raj Kanakaiprath
*******************************************
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/types.h>
#include <semaphore.h>
#include <time.h>
#include <linux/input.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <sys/resource.h>
#include "thread_model.h"

#define NUM_EVENTS 10                  // Define the number of interrupts
#define TIME_SPEC_LIMIT 1000000000      // Define LIMIT for time_spec members
#define MS_TO_NS 1000000                // Multiply to convert Milliseconds to Nanoseconds

pthread_mutex_t mutex[NUM_MUTEXES]; //mutexes
pthread_barrier_t barrier; //barrier
pthread_cond_t cond_var[NUM_EVENTS]; //conditional variables
pthread_mutex_t dummy_mutex[NUM_EVENTS] = {PTHREAD_MUTEX_INITIALIZER}; //Dummy mutex for conditional variable use

//Set task flag to TRUE
static int flag = TRUE;

// Set policy to SCHED_FIFO
int policy=SCHED_FIFO;

//Keyboard thread routine
void *keyborad_thread(void *arg)
{
        int fd, event, old_type;
        struct input_event ie;
        //ENABLE ASYNCHRONUS CANCELLATION 
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &old_type);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        //Check if keyboard file opened succesfully
        if((fd = open(KEYBOARD_EVENT_DEV,O_RDONLY))== -1)
        {
            printf("opening keyboard file failed\n");
        }
        //Wait for all threads to start simultaneously
        pthread_barrier_wait(&barrier);
        while(flag)
        {
            //Read keyboard file
            if(read(fd, &ie, sizeof(struct input_event)))
            {
                //Broadcast conditional variable depending on the key pressed
                if((ie.type ==1)&&(ie.code == 11)&&(ie.value == 0))
                {
                    pthread_cond_broadcast(&cond_var[0]);
                }
                for(event =1; event<NUM_EVENTS; event++)
                {
                    if((ie.type ==1)&&(ie.code == event + 1)&&(ie.value == 0))
                    {
                        pthread_cond_broadcast(&cond_var[event]);                
                    }
                }
            }
        }  
        
        
    close(fd);    
    pthread_exit(NULL);
}

//Periodic routine for creating periodic threads
void *periodic_routine(void *arg)
{
    struct Tasks thread_data = *((struct Tasks *)arg);
    struct timespec next,period;
    period.tv_nsec = thread_data.period * MS_TO_NS;
    volatile uint64_t x;
    //Wait for execution to start
    pthread_barrier_wait(&barrier);                    
    clock_gettime(CLOCK_MONOTONIC, &next); //Get time
    while(flag)
    {
        x = thread_data.loop_iter[0];
        while(x>0) x--; //<compute1>
        pthread_mutex_lock(&mutex[thread_data.mutex_num]);
        x = thread_data.loop_iter[1];
        while(x>0) x--; //<compute2>
        pthread_mutex_unlock(&mutex[thread_data.mutex_num]);
        x = thread_data.loop_iter[2]; 
        while(x>0) x--;  //<compute3>
              
        next.tv_sec = next.tv_sec + ((next.tv_nsec + period.tv_nsec)/TIME_SPEC_LIMIT); //check for limit
        next.tv_nsec = ((next.tv_nsec + period.tv_nsec)%TIME_SPEC_LIMIT) ;  //check for limit
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next,0);  // <wait for period>
    }
    pthread_exit(NULL);
}

void *aperiodic_routine(void *arg)
{
    struct Tasks thread_data = *((struct Tasks *)arg);
    volatile uint64_t x;
    //Wait for execution to start
    pthread_barrier_wait(&barrier);
    pthread_cond_wait(&cond_var[thread_data.event_key],&dummy_mutex[thread_data.event_key]); // <wait for activation>
    
    while(flag)
    {
        //printf("received %d\n", thread_data.event_key);
        x = thread_data.loop_iter[0];
        while(x>0) x--; //<compute1>
        pthread_mutex_lock(&mutex[thread_data.mutex_num]);
        x = thread_data.loop_iter[1];
        while(x>0) x--; ////<compute2>
        pthread_mutex_unlock(&mutex[thread_data.mutex_num]);
        x = thread_data.loop_iter[2]; 
        while(x>0) x--;  //<compute3>
        pthread_cond_wait(&cond_var[thread_data.event_key],&dummy_mutex[thread_data.event_key]);   // <wait for activation> 
    }
    pthread_exit(NULL);
}

//Main thread
int main (int argc, char *argv[])
{
    pthread_t thread_id[NUM_THREADS + 1], main_id;
    int loop_iter; //local variable for looping
    struct sched_param param;
    int priority_max;
    int status;
    pthread_attr_t attr;    
    
    //Get thread ID of main()
    main_id = pthread_self();

    priority_max = sched_get_priority_max(policy);

    //Set priority of main to 99
    param.sched_priority=priority_max;
    status = pthread_setschedparam(main_id, policy, &param);
    if (status != 0) perror("pthread_setschedparam"); 

    //Initialize mutexes
    for(loop_iter=0;loop_iter<NUM_MUTEXES;loop_iter++)
    {
        pthread_mutex_init(&mutex[loop_iter], NULL);
    }

    //Initializing conditional variables
    for(loop_iter=0;loop_iter< NUM_EVENTS; loop_iter++){
        pthread_cond_init(&cond_var[loop_iter],NULL);
    }

    //Initialize barrier
    pthread_barrier_init(&barrier,NULL,NUM_THREADS+1);
    
    //Initialize attributes and set attribute properties
    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, policy);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);    
   

    //Set priority of keyboard thread
    param.sched_priority=priority_max-1;
    pthread_attr_setschedparam(&attr, &param);
    //Creating Keyboard thread
    if(pthread_create(&thread_id[NUM_THREADS],&attr, &keyborad_thread, NULL))
    {
        perror("keyboard thread creation failed\n");
        return 3;
    }

    //Create periodic and aperiodic threads with assigned priorities
    for (loop_iter=0;loop_iter<NUM_THREADS; loop_iter++)
    {
        param.sched_priority=threads[loop_iter].priority;
        pthread_attr_setschedparam(&attr, &param);
        if(threads[loop_iter].task_type==0)
        {
            if(pthread_create(&thread_id[loop_iter],&attr, &periodic_routine, (void *) &threads[loop_iter]))
            {
                perror("thread creation failed\n");
                return 1;
            }
        }
        else{
            if(pthread_create(&thread_id[loop_iter],&attr, &aperiodic_routine, (void *) &threads[loop_iter]))
            {
                perror("thread creation failed\n");
                return 2;
            }
        }
    }

    //Wait for TOTAL_TIME to elapse and set a the flag to FALSE
    sleep(TOTAL_TIME/1000);
    flag = FALSE;

    //Cancel Keyboard thread
    pthread_cancel(thread_id[NUM_THREADS]);

    //Signal aperiodic threads to become active
    for(loop_iter = 0;loop_iter<NUM_THREADS;loop_iter++)
    {
        if(threads[loop_iter].task_type)
        {
           pthread_cond_broadcast(&cond_var[threads[loop_iter].event_key]);  
        }
    }

    //Wait for all thread to properly terminate
    for(loop_iter=0; loop_iter< NUM_THREADS + 1; loop_iter++)
    {
        pthread_join(thread_id[loop_iter], NULL);
    }
    //Destroy all mutexes
    for(loop_iter=0;loop_iter<NUM_MUTEXES;loop_iter++)
    {
        pthread_mutex_destroy(&mutex[loop_iter]);
    }
    //Destroy Conditional Variables and the dummy mutexes
    for(loop_iter=0;loop_iter< NUM_EVENTS; loop_iter++){
        pthread_cond_destroy(&cond_var[loop_iter]);
         pthread_mutex_destroy(&dummy_mutex[loop_iter]);
    }
    pthread_barrier_destroy(&barrier); //Destroy all barrier
    pthread_attr_destroy(&attr); // Destroy attribute
    printf("Exiting from main \n"); 
    return EXIT_SUCCESS;
}

