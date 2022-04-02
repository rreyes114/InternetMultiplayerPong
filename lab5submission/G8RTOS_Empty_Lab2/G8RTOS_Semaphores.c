/*
 * G8RTOS_Semaphores.c
 */

/*********************************************** Dependencies and Externs *************************************************************/

#include <stdint.h>
#include "msp.h"
#include "G8RTOS_Semaphores.h"
#include "G8RTOS_CriticalSection.h"
#include "G8RTOS_Structures.h"

/*********************************************** Dependencies and Externs *************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Initializes a semaphore to a given value
 * Param "s": Pointer to semaphore
 * Param "value": Value to initialize semaphore to
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_InitSemaphore(semaphore_t *s, int32_t value)
{
	/* Implement this */
    int32_t IBit_State = StartCriticalSection();
    *s = value;
    EndCriticalSection(IBit_State);
}

/*
 * Waits for a semaphore to be available (value greater than 0)
 * 	- Decrements semaphore when available
 * 	- Spinlocks to wait for semaphore
 * Param "s": Pointer to semaphore to wait on
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_WaitSemaphore(semaphore_t *s)
{
	/* Implement this */
    int32_t IBit_State = StartCriticalSection(); //disable interrupts
    (*s)--;
    if((*s) < 0) {
        //block thread
        CurrentlyRunningThread->blocked = s;
        //yield
        SCB->ICSR |= (1 << 28); //setting PendSV flag to initiate context switch
    }
    EndCriticalSection(IBit_State);         //enable interrupts

    /* Lab 2 spinlock version
    int32_t IBit_State = StartCriticalSection();
    while(*s == 0) {
        EndCriticalSection(IBit_State);         //enable interrupts
        for (int i = 0; i < 20; i++);
        IBit_State = StartCriticalSection();    //disable interrupts
    }
    (*s)--;
    EndCriticalSection(IBit_State);         //enable interrupts
    */
}

/*
 * Signals the completion of the usage of a semaphore
 * 	- Increments the semaphore value by 1
 * Param "s": Pointer to semaphore to be signalled
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_SignalSemaphore(semaphore_t *s)
{
	/* Implement this */
    int32_t IBit_State = StartCriticalSection();
    (*s)++;
    if((*s) <= 0) { //check if there is a thread waiting on semaphore and unblock ONE of them
        tcb_t *pt = CurrentlyRunningThread->next;
        while(pt->blocked != s) {
            pt = pt->next;
        }
        pt->blocked = 0;
    }
    EndCriticalSection(IBit_State);         //enable interrupts

    /* Lab 2 spinlock version
    int32_t IBit_State = StartCriticalSection();
    (*s)++;
    EndCriticalSection(IBit_State);         //enable interrupts
    */
}

/*********************************************** Public Functions *********************************************************************/


