#include "msp.h"
#include <driverlib.h>
#include <BSP.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "G8RTOS.h"
#include "Game.h"
//#include "Threads.h"
#define JOYSTICKFIFO 1

semaphore_t WifiReady;
semaphore_t LCDReady;

GameState_t gameState;  //received from host
GeneralPlayerInfo_t playerInfo; //send to host
GeneralPlayerInfo_t player_array[MAX_NUM_OF_PLAYERS];
PrevPlayer_t prevPlayers[MAX_NUM_OF_PLAYERS];
PrevBall_t prevBalls[MAX_NUM_OF_BALLS];

threadId_t thread_IDs[MAX_THREADS];
uint8_t IDindex;

bool TPflag = false;

void startGame();
void LCDtap();
void testWifiConnection(bool sending);

/**
 * main.c
 */
void main(void)
{
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		// stop watchdog timer
    srand(time(NULL));

    G8RTOS_Init();
//    G8RTOS_InitSemaphore(&LCDReady,1);
    G8RTOS_AddThread(&startGame,0,"startGame");
    G8RTOS_AddAPeriodicEvent(&LCDtap,1,PORT4_IRQn);
    G8RTOS_Launch();
}

void startGame() {
    P4->IFG &= ~BIT0;
    P4->IE |= BIT0;

    G8RTOS_WaitSemaphore(&LCDReady);
    LCD_Text(40, 120,"Host Game",LCD_WHITE); //prompt
    LCD_Text(200, 120,"Join Game",LCD_WHITE); //prompt
    G8RTOS_SignalSemaphore(&LCDReady);

    while(!TPflag); //wait for touch
    P4->IE &= ~BIT0; //disable interrupt
    TPflag = false;

    Point point = TP_ReadXY(); //read touch point

    G8RTOS_WaitSemaphore(&LCDReady);
    LCD_Clear(LCD_BLACK);
    G8RTOS_SignalSemaphore(&LCDReady);

    if(point.x < 160){
        G8RTOS_AddThread(&CreateGame, 0, "CreateGame");
    }
    else{
        G8RTOS_AddThread(&JoinGame, 0, "JoinGame");
    }
    G8RTOS_KillSelf();
}

void LCDtap() {
    P4->IE &= ~BIT0; //disable interrupt
    //G8RTOS_SignalSemaphore(&TPReady);
    for(int i = 0; i < ClockSys_GetSysFreq()/4000; i++);

    TPflag = true;
    P4->IFG &= ~BIT0; // must clear IFG flag // reading PxIV will automatically clear IFG
    P4->IE |= BIT0; //re-enable interrupt
}

void testWifiConnection(bool sending) {
    _u32 my_ip_addr = getLocalIP();
    //bool sending = true;
    if(sending) {
      unsigned char *data = "hello Daniel"; //help
        //while(1) {
          SendData(data, HOST_IP_ADDR, 4 * sizeof(uint32_t));
          for(int i = 0; i < 100; i++);
        //}
    }
    else { //receiving
        unsigned char *tempData = "";//(uint8_t*)
        int32_t retval = ReceiveData(tempData, 2 * sizeof(uint32_t));
        while(retval < 0) {
            retval = ReceiveData(tempData, 2 * sizeof(uint32_t));
            for(int i = 0; i < 100; i++);
        }
        while(1);
    }
    while(1);
}
