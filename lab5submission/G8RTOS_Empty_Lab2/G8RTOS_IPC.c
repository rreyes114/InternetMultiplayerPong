/*
 * G8RTOS_IPC.c
 *
 *  Created on: Jan 10, 2017
 *      Author: Daniel Gonzalez
 */
#include <stdint.h>
#include "msp.h"
#include "G8RTOS_IPC.h"
#include "G8RTOS_Semaphores.h"

/*********************************************** Defines ******************************************************************************/

#define FIFOSIZE 16
#define MAX_NUMBER_OF_FIFOS 4

/*********************************************** Defines ******************************************************************************/


/*********************************************** Data Structures Used *****************************************************************/

/*
 * FIFO struct will hold
 *  - buffer
 *  - head
 *  - tail
 *  - lost data
 *  - current size
 *  - mutex
 */

/* Create FIFO struct here */
typedef struct FIFO_t{
    int32_t buffer[FIFOSIZE];
    int32_t * head;
    int32_t * tail;
    uint32_t lost_data;
    semaphore_t curr_size;
    semaphore_t mutex;
} FIFO_t;

/* Array of FIFOS */
static FIFO_t FIFOs[4];

/*********************************************** Data Structures Used *****************************************************************/

/*
 * Initializes FIFO Struct
 */
int G8RTOS_InitFIFO(uint32_t FIFOIndex)
{
    /* Implement this */
    if(FIFOIndex >= MAX_NUMBER_OF_FIFOS) {
        return -1; }

    FIFOs[FIFOIndex].head = &FIFOs[FIFOIndex].buffer[0];
    FIFOs[FIFOIndex].tail = &FIFOs[FIFOIndex].buffer[0]; //points to the first available space in buffer
    FIFOs[FIFOIndex].lost_data = 0;
    G8RTOS_InitSemaphore(&FIFOs[FIFOIndex].curr_size, 0);
    G8RTOS_InitSemaphore(&FIFOs[FIFOIndex].mutex, 1);

    return 0;
}

/*
 * Reads FIFO
 *  - Waits until CurrentSize semaphore is greater than zero
 *  - Gets data and increments the head ptr (wraps if necessary)
 * Param: "FIFOChoice": chooses which buffer we want to read from
 * Returns: uint32_t Data from FIFO
 */
uint32_t readFIFO(uint32_t FIFOChoice)
{
    /* Implement this */
    G8RTOS_WaitSemaphore(&FIFOs[FIFOChoice].curr_size);
    G8RTOS_WaitSemaphore(&FIFOs[FIFOChoice].mutex);
    uint32_t data = *FIFOs[FIFOChoice].head;
    if(FIFOs[FIFOChoice].head == &FIFOs[FIFOChoice].buffer[15]) {
        FIFOs[FIFOChoice].head = &FIFOs[FIFOChoice].buffer[0];
    }
    else {
        FIFOs[FIFOChoice].head++;
    }
    G8RTOS_SignalSemaphore(&FIFOs[FIFOChoice].mutex);
    return data;
}

/*
 * Writes to FIFO
 *  Writes data to Tail of the buffer if the buffer is not full
 *  Increments tail (wraps if ncessary)
 *  Param "FIFOChoice": chooses which buffer we want to read from
 *        "Data': Data being put into FIFO
 *  Returns: error code for full buffer if unable to write
 */
int writeFIFO(uint32_t FIFOChoice, uint32_t Data)
{
    /* Implement this */
    if(FIFOs[FIFOChoice].curr_size > FIFOSIZE-1) {
        FIFOs[FIFOChoice].lost_data++;
        return -1;
    }

    G8RTOS_WaitSemaphore(&FIFOs[FIFOChoice].mutex);
    *FIFOs[FIFOChoice].tail = Data;
    if(FIFOs[FIFOChoice].tail == &FIFOs[FIFOChoice].buffer[15]) {
        FIFOs[FIFOChoice].tail = &FIFOs[FIFOChoice].buffer[0];
    }
    else {
        FIFOs[FIFOChoice].tail++;
    }
    G8RTOS_SignalSemaphore(&FIFOs[FIFOChoice].mutex);
    G8RTOS_SignalSemaphore(&FIFOs[FIFOChoice].curr_size);

    return 0;
}

