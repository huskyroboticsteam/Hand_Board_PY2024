/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/

#include "FSM_Stuff.h"
#include "project.h"
#include "main.h"
#include <stdio.h>

uint8_t currentState  = UNINIT;
uint8_t currentMode   = 0xFF;

void GotoUninitState() {
    currentState = UNINIT;
    // reset any parameters
}
void SetStateTo(uint8_t state) {
    currentState = state;
    char txData[TX_DATA_SIZE];
    sprintf(txData, "State Set to %d\r\n", state);
    Print(txData);
}
void SetModeTo(uint8_t mode) {
    currentMode = mode;
    char txData[TX_DATA_SIZE];
    sprintf(txData, "Mode Set to %d\r\n", mode);
    Print(txData);
}
uint8_t GetState(){
    return currentState;
}
uint8_t GetMode(){
    return currentMode;
}

/* [] END OF FILE */