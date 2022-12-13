//#define DEBUG 

#if defined(DEBUG) 
	#define DPRINTF(fmt, args...) printf("DEBUG: %s:%d:%s(): " fmt, \
   		 __FILE__, __LINE__, __func__, ##args)
#else
 	#define DPRINTF(fmt, args...) /* do nothing if not defined*/
#endif

#define KEYBOARD_EVENT_DEV "/dev/input/event1"	


#define NUM_MUTEXES 3
#define NUM_THREADS 5
#define TOTAL_TIME 2000
#define TRUE 1
#define FALSE 0

struct Tasks
{
	int task_type;			// Periodic or aperiodic task
	int task_num;
	int event_key;			// Event number to poll on
	int priority;
	int period;				// Period of periodic task
	int loop_iter[3];			// loop iterations for computation
	int mutex_num;
};

#define THREAD0 {0, 0, 0, 70, 10, {500000, 200000, 50000}, 2}
#define THREAD1 {0, 1, 0, 65, 28, {500000, 200000, 50000}, 1}
#define THREAD2 {0, 2, 0, 64, 64, {500000, 200000, 50000}, 2}
#define THREAD3 {1, 3, 0, 80, 10, {500000, 200000, 50000}, 2}
#define THREAD4 {1, 4, 7, 85, 10, {500000, 200000, 50000}, 0}


struct Tasks threads[NUM_THREADS]={THREAD0, THREAD1, THREAD2, THREAD3, THREAD4};

