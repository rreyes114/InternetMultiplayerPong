/*
 * G8RTOS_Structure.h
 *
 *  Created on: Jan 12, 2017
 *      Author: Raz Aloni
 */

#ifndef G8RTOS_STRUCTURES_H_
#define G8RTOS_STRUCTURES_H_

#include "G8RTOS.h"
#include <stdbool.h> //added in lab 3

/*********************************************** Data Structure Definitions ***********************************************************/

/*
 *  Thread Control Block:
 *      - Every thread has a Thread Control Block
 *      - The Thread Control Block holds information about the Thread Such as the Stack Pointer, Priority Level, and Blocked Status
 *      - For Lab 2 the TCB will only hold the Stack Pointer, next TCB and the previous TCB (for Round Robin Scheduling)
 */

/* Create tcb struct here */
typedef struct tcb_t {
    int32_t * stk_ptr;
    struct tcb_t * next;
    struct tcb_t * prev;
    semaphore_t * blocked;
    uint32_t sleep_count;
    bool asleep;
    uint8_t priority;
    bool alive;
    char threadName[16];
    uint32_t threadID;
} tcb_t;

/*
 *  Periodic Thread Control Block:
 *      - Holds a function pointer that points to the periodic thread to be executed
 *      - Has a period in us
 *      - Holds Current time
 *      - Contains pointer to the next periodic event - linked list
 */

/* Create periodic thread struct here */
typedef struct ptcb_t {
    void (*Handler)(void);
    uint32_t period;
    uint32_t exec_time;
    //uint32_t curr_time;
    struct ptcb_t * prev;
    struct ptcb_t * next;
} ptcb_t;

/*********************************************** Data Structure Definitions ***********************************************************/


/*********************************************** Public Variables *********************************************************************/

tcb_t * CurrentlyRunningThread;

/*********************************************** Public Variables *********************************************************************/




#endif /* G8RTOS_STRUCTURES_H_ */
