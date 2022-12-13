/*
Assignment 3
Author:- Roshan Raj Kanakaiprath 
*/

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "object.h"



#define DEVICE_FILE_0                       "/dev/ht438_dev_0"
#define DEVICE_FILE_1                       "/dev/ht438_dev_1"

#define OUTPUT_FILE_1                       "t1_out"
#define OUTPUT_FILE_2                       "t2_out"

#define NUM_THREADS                         2

int fd0, fd1;

FILE *fin[2], *fout[2]; //for file operations on test scripts

char print_ret[4];
char *print_data(char *d)
{
    memset(print_ret, 0, sizeof(print_ret));

    for(int i=0; i<4; i++) {
        print_ret[i] = d[i];
    }

    return print_ret;
}

//Function to copy data
void data_copy(char *dest, char *src)
{
    for(int i=0; i<strlen(src); i++) {
        dest[i] = src[i];
    }
}

pthread_barrier_t barrier;
int policy=SCHED_FIFO;

void *test_thread(void *args)
{
    int thread_number = *(int *)args;

    printf("Thread: %d\n", thread_number);

    char operation = 0;
    char table_data[4] = {0};
    int table_num = 0;
    int table_key = 0;
    int sleep_time = 0;

    char buffer[100];

    pthread_barrier_wait(&barrier);

    while(fgets(buffer, sizeof(buffer), fin[thread_number]))
    {
        // printf("%s\n", buffer);
        ht_object_t object = {0};

        if(buffer[0] == 'w') {
            memset(table_data, 0, 4);
            sscanf(buffer, "%c %d %d %4s", &operation, &table_num, &table_key, table_data);
            object.key = table_key;
            data_copy(object.data, table_data);
            if(table_num == 0) {
                write(fd0, &object, sizeof(ht_object_t));
            }
            else if(table_num == 1) {
                write(fd1, &object, sizeof(ht_object_t));
            }
        }
        else if(buffer[0] == 'r') {
            char dummy_data[50] = {0};
            sscanf(buffer, "%c %d %d", &operation, &table_num, &table_key);
            object.key = table_key;
            if(table_num == 0) {
                read(fd0, &object, sizeof(ht_object_t));
            }
            else if(table_num == 1) {
                read(fd1, &object, sizeof(ht_object_t));
            }
            sprintf(dummy_data, "read from ht438_tbl_%d\nkey=%d\tdata=%s\n", table_num, object.key, print_data(object.data));
            fputs(dummy_data, fout[thread_number]);
        }
        else if(buffer[0] == 's') {
            sscanf(buffer, "%c %d", &operation, &sleep_time);
            printf("Sleep Time: %d us\n", sleep_time);
            usleep(sleep_time);
        }
    }

    pthread_exit(NULL);
}

int main(int argc, char **argv)
{
    int ret, thread0 = 0, thread1 = 1;

    if(argc < 3) {
        printf("Invalid no. of files\n");
        return -1;
    }

    printf("Opening File %s\n", DEVICE_FILE_0);
    fd0 = open(DEVICE_FILE_0, O_RDWR);
    if(fd0 < 0) {
        printf("Error opening %s\n", DEVICE_FILE_0);
        return -1;
    }

    printf("Opening file %s\n", DEVICE_FILE_1);
    fd1 = open(DEVICE_FILE_1, O_RDWR);
    if(fd1 < 0) {
        printf("Error opening %s\n", DEVICE_FILE_1);
        return -1;
    }

    fout[0] = fopen(OUTPUT_FILE_1, "w");
    if(fout[0] == NULL) {
        printf("Cannot open file %s\n", OUTPUT_FILE_1);
    }

    fout[1] = fopen(OUTPUT_FILE_2, "w");
    if(fout[1] == NULL) {
        printf("Cannot open file %s\n", OUTPUT_FILE_2);
    }

    fin[0] = fopen(argv[1], "r");
    if(fin[0] == NULL) {
        printf("Cannot open file %s\n", argv[1]);
    }

    fin[1] = fopen(argv[2], "r");
    if(fin[1] == NULL) {
        printf("Cannot open file %s\n", argv[2]);
    }

    pthread_t main_thread_id; //main thread ID

    pthread_t thread_id[NUM_THREADS]; //thread IDs

    //thread attributes and scheduling  parameters
    pthread_attr_t thread_attr;
    struct sched_param param;
    // Set main thread priority
    main_thread_id = pthread_self();
    param.sched_priority = 90;
    pthread_setschedparam(main_thread_id, SCHED_FIFO, &param);

    //Initialize thread attributes
    pthread_attr_init(&thread_attr);

    //Setting Scheduling policy to (SCHED_FIFO) Priority Based Scheduling
    pthread_attr_setschedpolicy(&thread_attr, policy);

    // As every thread has different priority, we use explicit sched params
    pthread_attr_setinheritsched(&thread_attr, PTHREAD_EXPLICIT_SCHED);

    pthread_barrier_init(&barrier, NULL, NUM_THREADS);

    param.sched_priority = 80;
    pthread_attr_setschedparam(&thread_attr, &param);
    pthread_create(&thread_id[0], &thread_attr, test_thread, &thread0);

    param.sched_priority = 70;
    pthread_attr_setschedparam(&thread_attr, &param);
    pthread_create(&thread_id[1], &thread_attr, test_thread, &thread1);

    for(int i=0; i<NUM_THREADS; i++)
    {
        pthread_join(thread_id[i], NULL);
        printf("Thread %d exited\n", i);
    }

    pthread_barrier_destroy(&barrier);

    pthread_attr_destroy(&thread_attr);

    printf("Exited all threads\n");
    
    ret = fclose(fin[0]);
    if(ret) {
        printf("Cannot close file %s\n", argv[1]);
    }

    ret = fclose(fin[1]);
    if(ret) {
        printf("Cannot close file%s\n", argv[2]);
    }

    ret = fclose(fout[0]);
    if(ret) {
        printf("Cannot close file%s\n", OUTPUT_FILE_1);
    }

    ret = fclose(fout[1]);
    if(ret) {
        printf("Cannot close file%s\n", OUTPUT_FILE_2);
    }

    ret = close(fd0);
    if(ret) {
        printf("Error closing %s\n", DEVICE_FILE_0);
    }

    ret = close(fd1);
    if(ret) {
        printf("Error closing %s\n", DEVICE_FILE_1);
    }

    return 0;
}