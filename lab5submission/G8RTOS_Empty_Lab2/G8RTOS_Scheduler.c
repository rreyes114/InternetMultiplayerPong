/*
 * G8RTOS_Scheduler.c
 */

/*********************************************** Dependencies and Externs *************************************************************/

#include <stdint.h>
#include "msp.h"
#include "G8RTOS_Scheduler.h"
#include "G8RTOS_Structures.h" //added due to error
#include <driverlib.h>         //added for InitSysTick
#include "BSP.h"               //added for G8RTOS_Init
#include "G8RTOS_CriticalSection.h" //added in lab 4 for AddThread

/*
 * G8RTOS_Start exists in asm
 */
extern void G8RTOS_Start();

/* System Core Clock From system_msp432p401r.c */
extern uint32_t SystemCoreClock;

/*
 * Pointer to the currently running Thread Control Block
 */
extern tcb_t * CurrentlyRunningThread;

/*********************************************** Dependencies and Externs *************************************************************/


/*********************************************** Defines ******************************************************************************/

/* Status Register with the Thumb-bit Set */
#define THUMBBIT 0x01000000

/*********************************************** Defines ******************************************************************************/


/*********************************************** Data Structures Used *****************************************************************/

/* Thread Control Blocks
 *	- An array of thread control blocks to hold pertinent information for each thread
 */
static tcb_t threadControlBlocks[MAX_THREADS];

/* Thread Stacks
 *	- An array of arrays that will act as individual stacks for each thread
 */
static int32_t threadStacks[MAX_THREADS][STACKSIZE];

/* Periodic Event Threads
 * - An array of periodic events to hold pertinent information for each thread
 */
static ptcb_t Pthread[MAXPTHREADS];

/*********************************************** Data Structures Used *****************************************************************/


/*********************************************** Private Variables ********************************************************************/

/*
 * Current Number of Threads currently in the scheduler
 */
static uint32_t NumberOfThreads;

/*
 * Current Number of Periodic Threads currently in the scheduler
 */
static uint32_t NumberOfPthreads;

/*
 * Current Number of Threads currently in the scheduler
 */
static uint16_t IDCounter;

/*********************************************** Private Variables ********************************************************************/


/*********************************************** Private Functions ********************************************************************/

/*
 * Initializes the Systick and Systick Interrupt
 * The Systick interrupt will be responsible for starting a context switch between threads
 * Param "numCycles": Number of cycles for each systick interrupt
 */
static void InitSysTick(uint32_t numCycles)
{
	/* Implement this */
    SysTick_Config(numCycles);
    SysTick_enableInterrupt();
}

/*
 * Chooses the next thread to run.
 * Lab 2 Scheduling Algorithm:
 * 	- Simple Round Robin: Choose the next running thread by selecting the currently running thread's next pointer
 * Lab 4 Priority Scheduler
 */
void G8RTOS_Scheduler()
{
	/* Implement This */
    tcb_t * tempNextThread = CurrentlyRunningThread->next;
    uint8_t highestPriority = UINT8_MAX;
    for(int i = 0; i < NumberOfThreads; i++) { //iterate through all threads
        if(tempNextThread->blocked == 0 && !tempNextThread->asleep) { //if thread is not blocked and not asleep
            if(tempNextThread->priority < highestPriority) {//if thread has higher priority than found so far
                CurrentlyRunningThread = tempNextThread;
                highestPriority = CurrentlyRunningThread->priority;
            }
        }
        tempNextThread = tempNextThread->next;
    }
}

/*
 * SysTick Handler
 * Currently the Systick Handler will only increment the system time
 * and set the PendSV flag to start the scheduler
 *
 * In the future, this function will also be responsible for sleeping threads and periodic threads
 */
void SysTick_Handler()
{
	/* Implement this */
    SystemTime++;

    //check for periodic threads
    ptcb_t * Pptr = &Pthread[0];
    for(uint32_t i = 0; i < NumberOfPthreads; i++) {
        if(Pptr->exec_time == SystemTime) {
            Pptr->exec_time = Pptr->period + SystemTime;
            Pptr->Handler(); //call function handler?
        }
        Pptr = Pptr->next;
    }

    //check for sleeping threads
    tcb_t * ptr = CurrentlyRunningThread;
    for(uint32_t i = 0; i < NumberOfThreads; i++) {
        if(ptr->asleep && ptr->sleep_count<=SystemTime) {
            ptr->asleep = false;
        }
        ptr = ptr->next;
    }

    SCB->ICSR |= (1 << 28); //setting PendSV flag (alternatively, use NVIC)
    //NVIC_EnableIRQ(PendSV_IRQn);
}

/*********************************************** Private Functions ********************************************************************/


/*********************************************** Public Variables *********************************************************************/

/* Holds the current time for the whole System */
uint32_t SystemTime;

/*********************************************** Public Variables *********************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Sets variables to an initial state (system time and number of threads)
 * Enables board for highest speed clock and disables watchdog
 */
void G8RTOS_Init()
{
	/* Implement this */
    SystemTime = 0;
    NumberOfThreads = 0;

    // Relocate vector table to SRAM to use aperiodic events (added in lab 4)
    uint32_t newVTORTable = 0x20000000;
    memcpy((uint32_t *)newVTORTable, (uint32_t *)SCB->VTOR, 57*4);
    // 57 interrupt vectors to copy
    SCB->VTOR = newVTORTable;

    bool usingSensorPack = false;
    if(usingSensorPack) {
        BSP_InitBoard();
    }
    else {
        WDT_A_clearTimer();
        WDT_A_holdTimer();
        ClockSys_SetMaxFreq();
//        initI2C();
//        sensorOpt3001Enable(true);
//        sensorTmp007Enable(true);
//        bmi160_initialize_sensor();
        Joystick_Init_Without_Interrupt();
//        bme280_initialize_sensor();
        BackChannelInit();
        init_RGBLEDS();
        LCD_Init(1);
    }
}

/*
 * Starts G8RTOS Scheduler
 * 	- Initializes the Systick
 * 	- Sets Context to first thread
 * Returns: Error Code for starting scheduler. This will only return if the scheduler fails
 */
int G8RTOS_Launch()
{
	/* Implement this */
    //CurrentlyRunningThread = &(threadControlBlocks[0]);
    tcb_t * tempNextThread = &(threadControlBlocks[0]);
    uint8_t highestPriority = UINT8_MAX;
    for(int i = 0; i < NumberOfThreads; i++) { //iterate through all threads
        if(tempNextThread->priority < highestPriority) {//if thread has higher priority than found so far
            CurrentlyRunningThread = tempNextThread;
            highestPriority = CurrentlyRunningThread->priority;
        }
        tempNextThread = tempNextThread->next;
    }
    InitSysTick(ClockSys_GetSysFreq()/1000);
    NVIC_SetPriority(SysTick_IRQn, 7);
    NVIC_SetPriority(PendSV_IRQn, 7);
    G8RTOS_Start();
    return -1; //error code?
}


/*
 * Adds threads to G8RTOS Scheduler
 * 	- Checks if there are stil available threads to insert to scheduler
 * 	- Initializes the thread control block for the provided thread
 * 	- Initializes the stack for the provided thread to hold a "fake context"
 * 	- Sets stack tcb stack pointer to top of thread stack
 * 	- Sets up the next and previous tcb pointers in a round robin fashion
 * Param "threadToAdd": Void-Void Function to add as preemptable main thread
 * Returns: Error code for adding threads
 */
int G8RTOS_AddThread(void (*threadToAdd)(void), uint8_t priority, char *name)
{
    int32_t IBit_State = StartCriticalSection();

    //check for available slots
    if(NumberOfThreads == MAX_THREADS)
        return -1; //error code

    //find first slot containing dead thread
    int newTCBindex = -1;
    for(int i = 0; i < MAX_THREADS; i++) {
        if(!threadControlBlocks[i].alive) {
            newTCBindex = i; break; }
    }
    if(newTCBindex == -1)
        return -1; //error code

    //initializing stack with dummy values
    for(int i = -16; i < -2; i++) {
        threadStacks[newTCBindex][STACKSIZE+i] = 0;
    }
    threadStacks[newTCBindex][STACKSIZE-2] = (int32_t)threadToAdd; //PC
    threadStacks[newTCBindex][STACKSIZE-1] = THUMBBIT; //PSR

    //set tcb stack pointer to top of thread stack, initialize other values
    threadControlBlocks[newTCBindex].stk_ptr = &(threadStacks[newTCBindex][STACKSIZE-16]);
    threadControlBlocks[newTCBindex].blocked = 0;
    threadControlBlocks[newTCBindex].asleep = false;
    threadControlBlocks[newTCBindex].priority = priority;
    threadControlBlocks[newTCBindex].alive = true;
    threadControlBlocks[newTCBindex].threadID = ((IDCounter++) << 16) | (uint32_t)&threadControlBlocks[newTCBindex];
    int i = 0;
    while(*name){
        threadControlBlocks[newTCBindex].threadName[i] = *name;
        name++; i++;
    }

    //point next to closest alive thread forward
    int nextAlive = newTCBindex+1;
    for(int i = 0; i < MAX_THREADS; i++) {
        nextAlive %= MAX_THREADS;
        if(threadControlBlocks[nextAlive].alive) {
            threadControlBlocks[newTCBindex].next = &threadControlBlocks[nextAlive]; break; }
        nextAlive++;
    }

    //point prev to closest alive thread backward
    int prevAlive = newTCBindex-1;
    for(int i = 0; i < MAX_THREADS; i++) {
        if(prevAlive < 0)
            prevAlive = MAX_THREADS-1;
        if(threadControlBlocks[prevAlive].alive) {
            threadControlBlocks[newTCBindex].prev = &threadControlBlocks[prevAlive]; break; }
        prevAlive--;
    }

    //adjust pointers of adjacent TCBs
    threadControlBlocks[newTCBindex].prev->next = &(threadControlBlocks[newTCBindex]);
    threadControlBlocks[newTCBindex].next->prev = &(threadControlBlocks[newTCBindex]);

    //increment # of threads
    NumberOfThreads++;

    EndCriticalSection(IBit_State);
    return 0;
}


/*
 * Adds periodic threads to G8RTOS Scheduler
 * Function will initialize a periodic event struct to represent event.
 * The struct will be added to a linked list of periodic events
 * Param Pthread To Add: void-void function for P thread handler
 * Param period: period of P thread to add
 * Returns: Error code for adding threads
 */
int G8RTOS_AddPeriodicEvent(void (*PthreadToAdd)(void), uint32_t period)
{
    int32_t IBit_State = StartCriticalSection();
    //check for available slots
    if(NumberOfPthreads == MAXPTHREADS) {
        return -1; } //error code?

    //initialize a new periodic tcb
    ptcb_t newPTCB;

    //assign ptcb next and prev in round robin fashion
    if(NumberOfPthreads == 0) {
        newPTCB.prev = &(Pthread[0]); }
    else {
        newPTCB.prev = &(Pthread[NumberOfPthreads-1]); }
    newPTCB.next = &(Pthread[0]); //next is always the first
    newPTCB.Handler = PthreadToAdd;
    newPTCB.period = period;
    newPTCB.exec_time = NumberOfPthreads+1; //not sure what to init to, made it a small offset to avoid common multiples

    Pthread[NumberOfPthreads] = newPTCB;
    if(NumberOfPthreads > 0) {
        Pthread[NumberOfPthreads-1].next = &(Pthread[NumberOfPthreads]);
        Pthread[0].prev = &(Pthread[NumberOfPthreads]);
    }

    //increment number of periodic events, exit
    NumberOfPthreads++;

    EndCriticalSection(IBit_State);
    return 0;
}


/*
 * Puts the current thread into a sleep state.
 *  param durationMS: Duration of sleep time in ms
 */
void sleep(uint32_t durationMS)
{
    /* Implement this */
    CurrentlyRunningThread->sleep_count = durationMS + SystemTime;
    CurrentlyRunningThread->asleep = true;
    SCB->ICSR |= (1 << 28); //setting PendSV flag to initiate context switch
}

/*
 * function that returns the CurrentlyRunningThread’s thread ID
 */
threadId_t G8RTOS_GetThreadId()
{
    return CurrentlyRunningThread->threadID;
}

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
sched_ErrCode_t G8RTOS_KillThread(threadId_t threadId)
{
    int32_t IBit_State = StartCriticalSection();

    //can't kill if only one thread running
    if(NumberOfThreads==1)
        return CANNOT_KILL_LAST_THREAD;

    //iterate through all the alive threads
    tcb_t * currTCB = CurrentlyRunningThread;
    bool matchFound = false;
    for(int i = 0; i < NumberOfThreads; i++) {
        if(currTCB->threadID == threadId) {
            matchFound = true; break; }
        currTCB = currTCB->next;
    }

    //return error code if no match found
    if(!matchFound)
        return THREAD_DOES_NOT_EXIST;

    //killing thread (set alive to false, and make adj TCBs point to each other)
    currTCB->alive = false;
    currTCB->prev->next = currTCB->next;
    currTCB->next->prev = currTCB->prev;

    //decrement # of threads
    NumberOfThreads--;

    EndCriticalSection(IBit_State);
    //initiate context switch if we're killing CurrentlyRunningThread
    if(currTCB == CurrentlyRunningThread)
        SCB->ICSR |= (1 << 28); //setting PendSV flag to initiate context switch

    return NO_ERROR;
}

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
sched_ErrCode_t G8RTOS_KillSelf()
{
    return G8RTOS_KillThread(CurrentlyRunningThread->threadID);
}

/*
 * The AddAPeriodicEvent function will be as follows:
 * - Verify the IRQn is less than the last exception (PSS_IRQn) and greater than
 *   last acceptable user IRQn (PORT6_IRQn), or else return appropriate error
 * - Verify priority is not greater than 6, the greatest user priority number,
 *   or else return appropriate error
 * - Use the following core_cm4 library functions to initialize the NVIC registers:
 *   o __NVIC_SetVector
 *   o __NVIC_SetPriority
 *   o NVIC_EnableIRQ
 */
sched_ErrCode_t G8RTOS_AddAPeriodicEvent(void (*AthreadToAdd)(void), uint8_t priority, IRQn_Type IRQn)
{
    int32_t IBit_State = StartCriticalSection();
    //check validity of IRQn
    if(IRQn < PSS_IRQn || IRQn > PORT6_IRQn)
        return IRQn_INVALID;

    //check if priority is valid
    if(priority > 6)
        return HWI_PRIORITY_INVALID;

    NVIC_SetVector(IRQn, (uint32_t)AthreadToAdd);
    NVIC_SetPriority(IRQn, priority);
    NVIC_EnableIRQ(IRQn);

    EndCriticalSection(IBit_State);
    return NO_ERROR;
}

/*********************************************** Public Functions *********************************************************************/
