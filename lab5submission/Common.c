/*
 * Common.c
 *
 *  Created on: Apr 23, 2021
 *      Author: Roehl
 */
#include <BSP.h>
#include "Game.h"

/*
 * Returns either Host or Client depending on button press
 */
//playerType GetPlayerRole() {
//    playerType fixError;
//    fixError.
//    return fixError;
//}

/*
 * Draw players given center X center coordinate
 */
void DrawPlayer(GeneralPlayerInfo_t * player){
    if(player->position == TOP){
        LCD_DrawRectangle(ARENA_MIN_X, ARENA_MIN_X + PADDLE_WID, player->currentCenter - PADDLE_LEN_D2, player->currentCenter + PADDLE_LEN_D2, player->color);
    }
    else{
        LCD_DrawRectangle(ARENA_MAX_X - PADDLE_WID , ARENA_MAX_X, player->currentCenter - PADDLE_LEN_D2, player->currentCenter + PADDLE_LEN_D2, player->color);
    }


}

/*
 * Updates player's paddle based on current and new center
 */
void UpdatePlayerOnScreen(PrevPlayer_t * prevPlayerIn, GeneralPlayerInfo_t * outPlayer) {
    int16_t prev_top = prevPlayerIn->Center + PADDLE_LEN_D2;
    int16_t new_bottom = outPlayer->currentCenter - PADDLE_LEN_D2;

    int16_t prev_bottom = prevPlayerIn->Center - PADDLE_LEN_D2;
    int16_t new_top = outPlayer->currentCenter + PADDLE_LEN_D2;



    int16_t x1 = 0;
    int16_t x2 = 0;
    if(outPlayer->position == TOP){
        x1 = ARENA_MIN_X;
        x2 = ARENA_MIN_X + PADDLE_WID;
    }
    else{
        x1 = ARENA_MAX_X - PADDLE_WID;
        x2 = ARENA_MAX_X;
    }
    //Moving Down and overlap
//    G8RTOS_WaitSemaphore(&WifiReady);
    G8RTOS_WaitSemaphore(&LCDReady);
    prevPlayerIn->Center = outPlayer->currentCenter;
    if(prev_top > new_bottom && prev_top < new_top){
        LCD_DrawRectangle(x1, x2, prev_bottom, new_bottom, LCD_BLACK);
        LCD_DrawRectangle(x1, x2, prev_top, new_top, outPlayer->color);
    }
    else if(prev_bottom < new_top && prev_bottom > new_bottom){
        LCD_DrawRectangle(x1, x2, new_top, prev_top, LCD_BLACK);
        LCD_DrawRectangle(x1, x2, new_bottom, prev_bottom, outPlayer->color);
    }
    else if(prevPlayerIn->Center != outPlayer->currentCenter){
        LCD_DrawRectangle(x1, x2, outPlayer->currentCenter - PADDLE_LEN_D2, outPlayer->currentCenter + PADDLE_LEN_D2, outPlayer->color);
        LCD_DrawRectangle(x1, x2, prevPlayerIn->Center - PADDLE_LEN_D2, prevPlayerIn->Center + PADDLE_LEN_D2, LCD_BLACK);
    }
    G8RTOS_SignalSemaphore(&LCDReady);
//    G8RTOS_SignalSemaphore(&WifiReady);
//    else if(prevPlayerIn->Center == outPlayer->currentCenter){
//        LCD_DrawRectangle(x1, x2, outPlayer->currentCenter - PADDLE_LEN_D2, outPlayer->currentCenter + PADDLE_LEN_D2, outPlayer->color);
//        prevPlayerIn->Center = outPlayer->currentCenter;
//    }

}

/*
 * Function updates ball position on screen
 */
void UpdateBallOnScreen(PrevBall_t * previousBall, Ball_t * currentBall, uint16_t outColor) {

    //erase old position
    LCD_DrawRectangle(previousBall->CenterX, previousBall->CenterX+2, previousBall->CenterY, previousBall->CenterY+2, LCD_BLACK);

    previousBall->CenterX = currentBall->currentCenterX;
    previousBall->CenterY = currentBall->currentCenterY;

    //draw new position
    LCD_DrawRectangle(currentBall->currentCenterX,currentBall->currentCenterX+2,currentBall->currentCenterY,currentBall->currentCenterY+2, outColor);


}

/*
 * Initializes and prints initial game state
 */
void InitBoardState() {
    LCD_Clear(LCD_BLACK);
//    LCD_DrawRectangle(ARENA_MIN_X, ARENA_MAX_X, ARENA_MIN_Y, ARENA_MAX_Y, LCD_WHITE);

    DrawPlayer(&gameState.players[HOST]);
    DrawPlayer(&gameState.players[CLIENT]);
}

/*
 * Idle thread
 */
void IdleThread() {
    thread_IDs[IDindex] = G8RTOS_GetThreadId();
    IDindex++;
    while(1);
}

/*
 * Thread to draw all the objects in the game
 */
void DrawObjects(){
    thread_IDs[IDindex] = G8RTOS_GetThreadId();
    IDindex++;
    while(1) {

        for(int i = 0; i < MAX_NUM_OF_BALLS; i++) {
            if(gameState.balls[i].alive){
//                G8RTOS_WaitSemaphore(&WifiReady);
                G8RTOS_WaitSemaphore(&LCDReady);

                UpdateBallOnScreen(&prevBalls[i], &gameState.balls[i], gameState.balls[0].color);

                G8RTOS_SignalSemaphore(&LCDReady);
//                G8RTOS_SignalSemaphore(&WifiReady);
            }
        }


        for(int i = 0; i < MAX_NUM_OF_PLAYERS; i++) {
            UpdatePlayerOnScreen(&prevPlayers[i], &gameState.players[i]);
        }



        sleep(20);
    }

}

/*
 * Thread to update LEDs based on score
 */
void MoveLEDs(){
    thread_IDs[IDindex] = G8RTOS_GetThreadId();
    IDindex++;
    while(1) {
        if(gameState.LEDScores[HOST] == 0) {
            LP3943_LedModeSet(BLUE, 0x00);
        }
        else if(gameState.LEDScores[HOST] == 1) {
            LP3943_LedModeSet(BLUE, 0x01);
        }
        else if(gameState.LEDScores[HOST] == 2) {
            LP3943_LedModeSet(BLUE, 0x03);
        }
        else if(gameState.LEDScores[HOST] == 3) {
            LP3943_LedModeSet(BLUE, 0x07);
        }
        else if(gameState.LEDScores[HOST] == 4) {
            LP3943_LedModeSet(BLUE, 0x0F);
        }
        else if(gameState.LEDScores[HOST] == 5) {
            LP3943_LedModeSet(BLUE, 0x1F);
        }
        else if(gameState.LEDScores[HOST] == 6) {
            LP3943_LedModeSet(BLUE, 0x3F);
        }
        else if(gameState.LEDScores[HOST] == 7) {
            LP3943_LedModeSet(BLUE, 0x7F);
        }
        else if(gameState.LEDScores[HOST] == 8) {
            LP3943_LedModeSet(BLUE, 0xFF);
        }

        if(gameState.LEDScores[CLIENT] == 0) {
            LP3943_LedModeSet(RED, 0x00);
        }
        else if(gameState.LEDScores[CLIENT] == 1) {
            LP3943_LedModeSet(RED, 0x8000);
        }
        else if(gameState.LEDScores[CLIENT] == 2) {
            LP3943_LedModeSet(RED, 0xC000);
        }
        else if(gameState.LEDScores[CLIENT] == 3) {
            LP3943_LedModeSet(RED, 0xE000);
        }
        else if(gameState.LEDScores[CLIENT] == 4) {
            LP3943_LedModeSet(RED, 0xF000);
        }
        else if(gameState.LEDScores[CLIENT] == 5) {
            LP3943_LedModeSet(RED, 0xF800);
        }
        else if(gameState.LEDScores[CLIENT] == 6) {
            LP3943_LedModeSet(RED, 0xFC00);
        }
        else if(gameState.LEDScores[CLIENT] == 7) {
            LP3943_LedModeSet(RED, 0xFE00);
        }
        else if(gameState.LEDScores[CLIENT] == 8) {
            LP3943_LedModeSet(RED, 0xFF00);
        }
        sleep(20);
    }
}
