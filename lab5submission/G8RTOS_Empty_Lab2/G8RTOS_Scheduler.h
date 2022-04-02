/*
 * G8RTOS_Scheduler.h
 */

#ifndef G8RTOS_SCHEDULER_H_
#define G8RTOS_SCHEDULER_H_
#include "msp.h"//added in lab 5 due to unseen error??

/*********************************************** Sizes and Limits *********************************************************************/
#define MAX_THREADS 15 //6 //changed in lab 4
#define MAXPTHREADS 6
#define STACKSIZE 128 //1024 //changed in lab 4
#define OSINT_PRIORITY 7
/*********************************************** Sizes and Limits *********************************************************************/

/*********************************************** Public Variables *********************************************************************/

/* Holds the current time for the whole System */
extern uint32_t SystemTime;

/*********************************************** Public Variables *********************************************************************/

/*********************************************** Datatype Definitions *****************************************************************/

/*
 * Lab 4 threadID typedef
 */
typedef uint32_t threadId_t;

/*
* Error Codes for Scheduler
*/
typedef enum
{
    NO_ERROR = 0,
    THREAD_LIMIT_REACHED = -1,
    NO_THREADS_SCHEDULED = -2,
    THREADS_INCORRECTLY_ALIVE = -3,
    THREAD_DOES_NOT_EXIST = -4,
    CANNOT_KILL_LAST_THREAD = -5,
    IRQn_INVALID = -6,
    HWI_PRIORITY_INVALID = -7
} sched_ErrCode_t;

/*********************************************** Datatype Definitions *****************************************************************/

/*********************************************** Public Functions *********************************************************************/

/*
 * Initializes variables and hardware for G8RTOS usage
 */
void G8RTOS_Init();

/*
 * Starts G8RTOS Scheduler
 * 	- Initializes Systick Timer
 * 	- Sets Context to first thread
 * Returns: Error Code for starting scheduler. This will only return if the scheduler fails
 */
int32_t G8RTOS_Launch();

/*
 * Adds threads to G8RTOS Scheduler
 * 	- Checks if there are stil available threads to insert to scheduler
 * 	- Initializes the thread control block for the provided thread
 * 	- Initializes the stack for the provided thread
 * 	- Sets up the next and previous tcb pointers in a round robin fashion
 * Param "threadToAdd": Void-Void Function to add as preemptable main thread
 * Returns: Error code for adding threads
 */
int32_t G8RTOS_AddThread(void (*threadToAdd)(void), uint8_t priority, char *name);


/*
 * Adds periodic threads to G8RTOS Scheduler
 * Function will initialize a periodic event struct to represent event.
 * The struct will be added to a linked list of periodic events
 * Param Pthread To Add: void-void function for P thread handler
 * Param period: period of P thread to add
 * Returns: Error code for adding threads
 */
int G8RTOS_AddPeriodicEvent(void (*PthreadToAdd)(void), uint32_t period);


/*
 * Puts the current thread into a sleep state.
 *  param durationMS: Duration of sleep time in ms
 */
void sleep(uint32_t durationMS);

/*
 * function that returns the CurrentlyRunningThread’s thread ID
 */
threadId_t G8RTOS_GetThreadId();

/*
 * The KillThread function will take in a threadId,
 * indicating the thread to kill. The implementation will
 * be as follows:
 * - Enter a critical section
 * - Return appropriate error code if there’s only one thread running
 * - Search for thread with the same threadId
 * - Return error code if the thread does not exist
 * - Set the threads isAlive bit to false
 * - Update thread pointers
 * - If thread being killed is the currently running thread, we need to context switch once critical section is ended
 * - Decrement number of threads
 * - End critical section
 */
sched_ErrCode_t G8RTOS_KillThread(threadId_t threadId);

/*
 * The KillSelf function will simply kill the currently
 * running thread. The implementation will be as
 * follows:
 * - Enter a critical section
 * - If only 1 thread running, return appropriate error code
 * - Change isAlive bit to false
 * - Update thread pointers
 * - Start context switch
 * - Decrement number of threads
 * - End critical section
 */
sched_ErrCode_t G8RTOS_KillSelf();

/*
 * The implementation will be as follows:
 * - Verify the IRQn is less than the last exception (PSS_IRQn) and greater than
 *   last acceptable user IRQn (PORT6_IRQn), or else return appropriate error
 * - Verify priority is not greater than 6, the greatest user priority number,
 *   or else return appropriate error
 * - Use the following core_cm4 library functions to initialize the NVIC registers:
 *   o __NVIC_SetVector
 *   o __NVIC_SetPriority
 *   o NVIC_EnableIRQ
 */
sched_ErrCode_t G8RTOS_AddAPeriodicEvent(void (*AthreadToAdd)(void), uint8_t priority, IRQn_Type IRQn);

/*********************************************** Public Functions *********************************************************************/

#endif /* G8RTOS_SCHEDULER_H_ */
