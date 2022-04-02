/*
 * Game.c
 *
 *  Created on: Apr 21, 2021
 *      Author: Anaconda
 */

#include "Game.h"
#include "LCDLib.h"
#include "G8RTOS.h"
#include "BSP.h"
#include "stdio.h"

//_u32 HOST_IP_ADDR = SL_IPV4_VAL(192,168,0,75);
extern SpecificPlayerInfo_t player_array[2];
int16_t global_xcoord, global_ycoord;

int16_t my_fifo[50];
uint8_t index;

int8_t ball_created_index = -1;


void CreateGame(){
    initCC3100(Client);
    G8RTOS_InitSemaphore(&WifiReady, 1);
    G8RTOS_InitSemaphore(&LCDReady, 1);

    for(uint8_t i = 0; i < MAX_NUM_OF_BALLS; i++){
        gameState.balls[i].alive = false;
        gameState.balls[i].color = LCD_GREEN;
        gameState.balls[i].currentCenterX = (ARENA_MAX_X + ARENA_MIN_X)/2;
        gameState.balls[i].currentCenterY = (ARENA_MAX_Y + ARENA_MIN_Y)/2;
        prevBalls[i].CenterX = (ARENA_MAX_X + ARENA_MIN_X)/2;
        prevBalls[i].CenterY = (ARENA_MAX_Y + ARENA_MIN_Y)/2;
    }

//    if(MAX_NUM_OF_BALLS > 0){
//        gameState.balls[0].alive = true;
//    }

//    uint32_t ip = getLocalIP();

//    player_array[HOST].IP_address = getLocalIP();
//    player_array[HOST].displacement = 0;
//    player_array[HOST].playerNumber = 0;
//    player_array[HOST].ready = true;
//    player_array[HOST].joined = true;
//    player_array[HOST].acknowledge = false;

    gameState.players[HOST].currentCenter = (ARENA_MAX_Y + ARENA_MIN_Y)/2;
    gameState.players[HOST].color = LCD_BLUE;
    gameState.players[HOST].position = TOP;

    _i32 ret_val = -1;

    SpecificPlayerInfo_t handshake;
    handshake.IP_address = 0;
    handshake.ready = false;
    handshake.joined = false;
    handshake.acknowledge = false;
    while(ret_val < 0 && handshake.IP_address != 0){
        ret_val = ReceiveData((uint8_t * )&handshake, sizeof(SpecificPlayerInfo_t) );
    }

    handshake.acknowledge = true;

    SendData((uint8_t * )&handshake, HOST_IP_ADDR, sizeof(SpecificPlayerInfo_t));
    while(ret_val < 0 && handshake.ready == false){
        ret_val = ReceiveData((uint8_t * )&handshake, sizeof(SpecificPlayerInfo_t) );
    }

    handshake.joined = true;

    SendData((uint8_t * )&handshake, HOST_IP_ADDR, sizeof(SpecificPlayerInfo_t));

    player_array[HOST].acknowledge = true;

    gameState.players[CLIENT].currentCenter = (ARENA_MAX_Y + ARENA_MIN_Y)/2;
    gameState.players[CLIENT].color = LCD_ORANGE;
    gameState.players[CLIENT].position = BOTTOM;

    gameState.player = player_array[CLIENT];
    gameState.gameDone = false;
    gameState.winner = false;
    gameState.LEDScores[0] = 0;
    gameState.LEDScores[1] = 0;
    gameState.overallScores[0] = 0;
    gameState.overallScores[1] = 0;
    gameState.numberOfBalls = 0;


//    SendData((uint8_t * )&gameState, HOST_IP_ADDR, sizeof(GameState_t));


    InitBoardState();

    G8RTOS_AddThread(&GenerateBall, 4, "GenerateBall");
    G8RTOS_AddThread(&DrawObjects, 3, "DrawObjects");
    G8RTOS_AddThread(&ReadJoystickHost, 5, "ReadJoystickHost");
    G8RTOS_AddThread(&SendDataToClient, 5, "SendDataToClient");
    G8RTOS_AddThread(&ReceiveDataFromClient, 5, "ReceiveDataFromClient");
    G8RTOS_AddThread(&MoveLEDs, 6, "MoveLEDs");
    G8RTOS_AddThread(&IdleThread, 7, "IdleThread");


    G8RTOS_KillSelf();
    while(1){

    }
}

void GenerateBall(){
    thread_IDs[IDindex] = G8RTOS_GetThreadId();
    IDindex++;
    //Adds another MoveBallthread if the number of balls is less than the max
    while(1){
        for(int i = 0; i < MAX_NUM_OF_BALLS; i++){
            if(gameState.balls[i].alive == false){
                gameState.numberOfBalls += 1;
                gameState.balls[i].alive = true;
                G8RTOS_AddThread(&MoveBall, 4, "ball_thread");
                ball_created_index = i;
                break;
            }
        }

        //Sleeps proportional to the number of balls currently in play
    //    sleep(alive_count * ADD_THREAD_SLEEP_TIME);
        sleep(2000);
    }


}

void MoveBall(){
    thread_IDs[IDindex] = G8RTOS_GetThreadId();
    IDindex++;
    if(ball_created_index >= 0){
        int8_t INDEX = ball_created_index;
        ball_created_index = -1;

        int8_t x_velocity = (rand() %  (2*MAX_BALL_SPEED + 1) ) - MAX_BALL_SPEED;
        if(x_velocity==0){
            x_velocity = 1;
        }


        int8_t y_velocity = (rand() %  (2*MAX_BALL_SPEED + 1)) - MAX_BALL_SPEED;
        if(y_velocity==0){
            y_velocity = 1;
        }
        while(1){
            //save old positions
//            prevBalls[i].CenterX = gameState.balls[i].currentCenterX;
//            prevBalls[i].CenterY = gameState.balls[i].currentCenterY;

            //check min boundary
            if(gameState.balls[INDEX].currentCenterY <= MAX_BALL_SPEED){
                gameState.balls[INDEX].currentCenterY += MAX_SCREEN_Y;
            }

            //update ball position
            gameState.balls[INDEX].currentCenterX += x_velocity;
            gameState.balls[INDEX].currentCenterY += y_velocity;

            //check max boundary
            if(gameState.balls[INDEX].currentCenterY >= ARENA_MAX_Y){
                gameState.balls[INDEX].currentCenterY %= ARENA_MAX_Y;
            }

            uint8_t minkowski_result1 = Minkowski(&gameState.balls[INDEX], &gameState.players[HOST]);
            uint8_t minkowski_result2 = Minkowski(&gameState.balls[INDEX], &gameState.players[CLIENT]);

            if(minkowski_result1 == EDGE_COLLISION || minkowski_result2 == EDGE_COLLISION){
                x_velocity *= -1;
            }
            else if(minkowski_result1 == CENTER_COLLISION || minkowski_result2 == CENTER_COLLISION){
                x_velocity *= -1;
                if(abs(x_velocity) <= 1){
                    x_velocity *= 2;
                }
                y_velocity = 0;
            }

            //score check
            if(gameState.balls[INDEX].currentCenterX <= ARENA_MIN_X + WIGGLE_ROOM ){
                gameState.LEDScores[CLIENT] += 1;

                if(gameState.LEDScores[CLIENT] >= 8 || gameState.LEDScores[HOST] >= 8){
                    gameState.gameDone = true;
                    SendData((uint8_t * )&gameState, HOST_IP_ADDR, sizeof(GameState_t));
                    G8RTOS_AddThread(&EndOfGameHost,0,"EndOfGameHost");

                    G8RTOS_KillSelf();
                }
                gameState.balls[INDEX].alive = false;
                gameState.balls[INDEX].currentCenterX = (ARENA_MAX_X + ARENA_MIN_X)/2;
                gameState.balls[INDEX].currentCenterY = (ARENA_MAX_Y + ARENA_MIN_Y)/2;
//                G8RTOS_WaitSemaphore(&LCDReady);
//                LCD_DrawRectangle(gameState.balls[INDEX].currentCenterX , gameState.balls[INDEX].currentCenterX + BALL_SIZE -1, gameState.balls[INDEX].currentCenterY,gameState.balls[INDEX].currentCenterY + BALL_SIZE -1, LCD_BLACK);
//                G8RTOS_SignalSemaphore(&LCDReady);
                gameState.numberOfBalls -= 1;
                G8RTOS_KillSelf();
            }
            else if(gameState.balls[INDEX].currentCenterX >= ARENA_MAX_X - WIGGLE_ROOM){
                gameState.LEDScores[HOST] += 1;

                if(gameState.LEDScores[CLIENT] >= 8 || gameState.LEDScores[HOST] >= 8){
                    gameState.gameDone = true;
                    SendData((uint8_t * )&gameState, HOST_IP_ADDR, sizeof(GameState_t));
                    G8RTOS_AddThread(&EndOfGameHost,0,"EndOfGameHost");

                    G8RTOS_KillSelf();
                }
                gameState.balls[INDEX].alive = false;
                gameState.balls[INDEX].currentCenterX = (ARENA_MAX_X + ARENA_MIN_X)/2;
                gameState.balls[INDEX].currentCenterY = (ARENA_MAX_Y + ARENA_MIN_Y)/2;
//                G8RTOS_WaitSemaphore(&LCDReady);
//                LCD_DrawRectangle(gameState.balls[INDEX].currentCenterX , gameState.balls[INDEX].currentCenterX + BALL_SIZE -1, gameState.balls[INDEX].currentCenterY,gameState.balls[INDEX].currentCenterY + BALL_SIZE -1, LCD_BLACK);
//                G8RTOS_SignalSemaphore(&LCDReady);
                gameState.numberOfBalls -= 1;
                G8RTOS_KillSelf();
            }


//            else if(gameState.balls[i].currentCenterY < ARENA_MIN_Y){
//                gameState.balls[i].currentCenterY += ARENA_MAX_Y;
//            }


            sleep(55);
        }
    }
}


collisionType_t Minkowski(Ball_t * ball, GeneralPlayerInfo_t * pl){
    int32_t w = 0.5 * (BALL_SIZE + PADDLE_WID);
    int32_t h = 0.5 * (BALL_SIZE + PADDLE_LEN);

    int32_t dx = 0;
    int32_t paddle_edge;
    if(pl->position == TOP){
        paddle_edge = PADDLE_WID + ARENA_MIN_X;
    }
    else{
        paddle_edge = ARENA_MAX_X - PADDLE_WID;
    }
    dx = ball->currentCenterX -1 - paddle_edge;
    int32_t dy = ball->currentCenterY -1 - pl->currentCenter;
    if (abs(dx) <= w && abs(dy) <= h){
        if(abs(dy) < 10){
            return CENTER_COLLISION;
        }
        return EDGE_COLLISION;
    }
    return NO_COLLISION;
}


void ReadJoystickHost(){
    thread_IDs[IDindex] = G8RTOS_GetThreadId();
    IDindex++;
    int16_t xcoord, ycoord;
    while(1) {
        GetJoystickCoordinates(&xcoord, &ycoord);

//        ycoord += JOYSTICK_BIAS;
//        player_array[HOST].displacement = global_xcoord - xcoord;
//        global_xcoord = xcoord;
        if(ycoord > 2000) {
            gameState.players[HOST].currentCenter += 2;
            if(gameState.players[HOST].currentCenter > 207)
                gameState.players[HOST].currentCenter = 206;
        }
        else if(ycoord < -2000) {
            gameState.players[HOST].currentCenter -= 2;
            if(gameState.players[HOST].currentCenter < 32)
                gameState.players[HOST].currentCenter = 32;
        }

        sleep(20);
    }
}
void SendDataToClient(){
    thread_IDs[IDindex] = G8RTOS_GetThreadId();
    IDindex++;
    while(1){
        G8RTOS_WaitSemaphore(&WifiReady);
        gameState.numberOfBalls += 1;
        SendData((uint8_t * )&gameState, HOST_IP_ADDR, sizeof(GameState_t));
        G8RTOS_SignalSemaphore(&WifiReady);
        //If done, Add EndOfGameHostthread with highest priority
        if(false){
            G8RTOS_AddThread(&EndOfGameHost, 0, "EndOfGameHost");
        }
        sleep(5);
    }

}
void EndOfGameHost(){
    SpecificPlayerInfo_t handshake;
    int32_t ret_val = -1;
    handshake.ready = false;
//    uint32_t exit_code = 69420;
//    uint32_t buffer = 0;
    while(ret_val < 0 || !handshake.ready){
        G8RTOS_WaitSemaphore(&WifiReady);
//        SendData((uint8_t * )&gameState, HOST_IP_ADDR, sizeof(GameState_t));
        ret_val = ReceiveData((uint8_t * )&handshake, sizeof(SpecificPlayerInfo_t));
        G8RTOS_SignalSemaphore(&WifiReady);
        sleep(2);//for(int i = 0; i < 100; i++);
    }

    G8RTOS_WaitSemaphore(&WifiReady);
    G8RTOS_WaitSemaphore(&LCDReady);

    //Kill threads
    for(int i = 0; i < IDindex; i++) {
        G8RTOS_KillThread(thread_IDs[i]);
    }
    IDindex = 0;



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

    gameState.LEDScores[HOST] = 0;
    gameState.LEDScores[CLIENT] = 0;


    G8RTOS_SignalSemaphore(&LCDReady);
    G8RTOS_SignalSemaphore(&WifiReady);

//
//    P4->IFG &= ~BIT0;
//    P4->IE |= BIT0;
//    TPflag = false;
//    while(!TPflag); //wait for touch
//    P4->IE &= ~BIT0; //disable interrupt
//    TPflag = false;
//
////    G8RTOS_InitSemaphore(&WifiReady,1);
////    G8RTOS_InitSemaphore(&LCDReady,1);
//
//    handshake.IP_address = 69420;
//    SendData((uint8_t * )&handshake, HOST_IP_ADDR, sizeof(SpecificPlayerInfo_t));
//


    G8RTOS_AddThread(&startGame, 0, "startGame");


//    G8RTOS_KillSelf();
    while(1) {
        sleep(200);
    }

}

void ReceiveDataFromClient(){
    thread_IDs[IDindex] = G8RTOS_GetThreadId();
    IDindex++;
    while(1){
        G8RTOS_WaitSemaphore(&WifiReady);
        _i32 ret_val = -1;

        while(ret_val < 0){
            ret_val = ReceiveData((uint8_t * )&gameState.players[CLIENT], sizeof(GeneralPlayerInfo_t) );
            G8RTOS_SignalSemaphore(&WifiReady);
            sleep(3);
            G8RTOS_WaitSemaphore(&WifiReady);
        }
        G8RTOS_SignalSemaphore(&WifiReady);

        sleep(2);


    }

}

