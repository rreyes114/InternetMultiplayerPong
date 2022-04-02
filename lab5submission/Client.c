/*
 * Client.c
 *
 *  Created on: Apr 21, 2021
 *      Author: Roehl
 */
#include <BSP.h>
#include <stdio.h>
#include <stdlib.h>
#include "Game.h"


//Move to common later
/*
 * Idle thread
 */

/*
 * Thread for client to join game
 */
void JoinGame() {
    initCC3100(Client);
    G8RTOS_InitSemaphore(&WifiReady,1);
    G8RTOS_InitSemaphore(&LCDReady,1);

    //Handshake code
    SpecificPlayerInfo_t newPlayer;
    //other values?
    newPlayer.IP_address = getLocalIP();

    //wait for acknowledge
    G8RTOS_WaitSemaphore(&LCDReady); //wait for acknowledge
    LCD_Text(10, 10,"Waiting for acknowledge",LCD_WHITE); //prompt
    G8RTOS_SignalSemaphore(&LCDReady);
    int32_t retval = -1;
    SendData((uint8_t*)&newPlayer, HOST_IP_ADDR, sizeof(SpecificPlayerInfo_t));
    while(retval < 0 && !newPlayer.acknowledge) {
        retval = ReceiveData((uint8_t*)&newPlayer, sizeof(SpecificPlayerInfo_t));
    }

    //send ready, wait for joined status
    G8RTOS_WaitSemaphore(&LCDReady);
    LCD_Text(10, 30,"Waiting for joined",LCD_WHITE); //prompt
    G8RTOS_SignalSemaphore(&LCDReady);
    newPlayer.ready = true;
    retval = -1;
    SendData((uint8_t*)&newPlayer, HOST_IP_ADDR, sizeof(SpecificPlayerInfo_t));
    while(retval < 0 && !newPlayer.joined) {
        retval = ReceiveData((uint8_t*)&newPlayer, sizeof(SpecificPlayerInfo_t));
    }

    //wait for first game state
//    retval = -1;
//    while(retval < 0) {
////        retval = ReceiveData((uint8_t*)&initState, sizeof(GameState_t));
//        retval = ReceiveData((uint8_t*)&gameState, sizeof(GameState_t));
//        for(int i = 0; i < 100; i++);
//    }

    //TODO extra stuff - delete later
    gameState.players[HOST].currentCenter = (ARENA_MAX_Y + ARENA_MIN_Y)/2;
    gameState.players[HOST].color = LCD_BLUE;
    gameState.players[HOST].position = TOP;
    gameState.players[CLIENT].currentCenter = (ARENA_MAX_Y + ARENA_MIN_Y)/2;
    gameState.players[CLIENT].color = LCD_ORANGE;
    gameState.players[CLIENT].position = BOTTOM;
    gameState.gameDone = false;
    gameState.winner = false;
    gameState.LEDScores[0] = 0;
    gameState.LEDScores[1] = 0;
    gameState.overallScores[0] = 0;
    gameState.overallScores[1] = 0;
    gameState.numberOfBalls = 0;

    playerInfo = gameState.players[CLIENT];
    prevBalls[0].CenterX = (ARENA_MAX_X + ARENA_MIN_X)/2;
    prevBalls[0].CenterY = (ARENA_MAX_Y + ARENA_MIN_Y)/2;

    InitBoardState();

    G8RTOS_AddThread(&IdleThread, 254, "Idle");
    G8RTOS_AddThread(&ReadJoystickClient, 2, "ReadJSClient");
    G8RTOS_AddThread(&SendDataToHost, 2, "SendDataHost");
    G8RTOS_AddThread(&ReceiveDataFromHost, 2, "RecvDataHost");
    G8RTOS_AddThread(&DrawObjects, 1, "DrawObjects");
    G8RTOS_AddThread(&MoveLEDs, 2, "MoveLEDs");

    G8RTOS_KillSelf();
}

uint16_t my_fifo[50];
int index;
/*
 * Thread that receives game state packets from host
 */
void ReceiveDataFromHost() {
    int32_t retval;
    thread_IDs[IDindex] = G8RTOS_GetThreadId();
    IDindex++;
    while(1) {
        retval = -1;

        while(retval < 0) {
            G8RTOS_WaitSemaphore(&WifiReady);   //grab semaphore
            retval = ReceiveData((uint8_t*)&gameState, sizeof(GameState_t));
            G8RTOS_SignalSemaphore(&WifiReady); //release semaphore
            sleep(1);
        }
        //TODO if gameState.gameDone, add EndOfGameClient
        if(gameState.gameDone)
            G8RTOS_AddThread(&EndOfGameClient, 0, "EndOfGameClient");
        if(index >= 50) {index=0;}
        my_fifo[index] = gameState.numberOfBalls;
        index++;

        sleep(5);
    }
}

/*
 * Thread that sends UDP packets to host
 */
void SendDataToHost() {
    thread_IDs[IDindex] = G8RTOS_GetThreadId();
    IDindex++;
    while(1) {
        //maybe read JoyStick FIFO and update playerInfo?
        //playerInfo.currentCenter += 1;
        G8RTOS_WaitSemaphore(&WifiReady);   //grab semaphore
        SendData((uint8_t*)&playerInfo, HOST_IP_ADDR, sizeof(GeneralPlayerInfo_t));
        G8RTOS_SignalSemaphore(&WifiReady); //release semaphore
        sleep(2);
    }
}

/*
 * Thread to read client's joystick
 */
void ReadJoystickClient() {
    thread_IDs[IDindex] = G8RTOS_GetThreadId();
    IDindex++;
//    G8RTOS_InitFIFO(JOYSTICKFIFO);        //might need FIFO if we want a better gradient of player movement.
    int16_t x_coord, y_coord;
    while(1) {
        GetJoystickCoordinates(&x_coord, &y_coord);
//        writeFIFO(JOYSTICKFIFO, y_coord); //Choppiness can come from not using a FIFO and from dropped packets
        if(y_coord > 2000) {
            playerInfo.currentCenter += 2;
            if(playerInfo.currentCenter > 207) //can go to 207, but increments of 2 mean 206
                playerInfo.currentCenter = 206;
//            gameState.players[CLIENT].currentCenter = playerInfo.currentCenter;
        }
        else if(y_coord < -2000) {
            playerInfo.currentCenter -= 2;
            if(playerInfo.currentCenter < 32)
                playerInfo.currentCenter = 32;
//            gameState.players[CLIENT].currentCenter = playerInfo.currentCenter;
        }
        sleep(10);
    }
}

/*
 * End of game for the client
 */
void EndOfGameClient() {
    SpecificPlayerInfo_t endPlayer;
//    endPlayer.ready = true;
//    uint32_t exit_code = 69420;

    endPlayer.ready = true;
    //wait for all semaphores
    G8RTOS_WaitSemaphore(&WifiReady);
    G8RTOS_WaitSemaphore(&LCDReady);

    SendData((uint8_t*)&endPlayer, HOST_IP_ADDR, sizeof(SpecificPlayerInfo_t));

    //kill threads
    for(int i = 0; i < IDindex; i++) {
        G8RTOS_KillThread(thread_IDs[i]);
    }
    IDindex = 0;

    //re-init semaphores
    G8RTOS_SignalSemaphore(&LCDReady);
    G8RTOS_SignalSemaphore(&WifiReady);
    G8RTOS_InitSemaphore(&WifiReady,1);
    G8RTOS_InitSemaphore(&LCDReady,1);

    //clear screen with winner color
    if(gameState.LEDScores[HOST] >= 8){
        LP3943_LedModeSet(BLUE, 0xFF);
        LCD_DrawRectangle(ARENA_MIN_X, ARENA_MAX_X, ARENA_MIN_Y, ARENA_MAX_Y, gameState.players[HOST].color);
        gameState.overallScores[HOST] += 1;
    }
    else{
        LP3943_LedModeSet(RED, 0xFF00);
        LCD_DrawRectangle(ARENA_MIN_X, ARENA_MAX_X, ARENA_MIN_Y, ARENA_MAX_Y, gameState.players[CLIENT].color);
        gameState.overallScores[CLIENT] += 1;
    }

    //wait for host to restart - wait for new packet I guess, maybe send a SpecificPlayerInfo for acknowledgement?
//    int32_t retval = -1;
//    endPlayer.acknowledge = false;
//    while(1) { //retval < 0 || endPlayer.acknowledge !=
//        retval = ReceiveData((uint8_t*)&endPlayer, sizeof(SpecificPlayerInfo_t));
//        if(retval == 0 && endPlayer.IP_address == 69420)
//            break;
//        for(int i = 0; i < 100; i++);
//    }
//
//    //re-initialize gameState
//    G8RTOS_AddThread(&JoinGame, 0, "JoinGame");
    G8RTOS_AddThread(&startGame,0,"startGame");

    //kill self
    G8RTOS_KillSelf();
}


