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

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "project.h"
#include "main.h"
#include "cyapicallbacks.h"
#include "CAN_Stuff.h"
#include "FSM_Stuff.h"
#include "HindsightCAN/CANLibrary.h"

// LED stuff
volatile uint8_t CAN_time_LED = 0;
volatile uint8_t ERROR_time_LED = 0;

#define SWITCH_DEBOUNCE_UNIT   (1u)

#define SWITCH_DEBOUNCE_PERIOD (10u)

#define SWITCH_RELEASED        (0u)
#define SWITCH_PRESSED         (1u)

// #define TX_DATA_SIZE           (25u)
#define ADC_CHANNEL_NUMBER_0   (0u)


static uint32 ReadSwSwitch(void);
CY_ISR_PROTO(ISR_CAN);

uint8 switchState = SWITCH_RELEASED;

volatile uint8 isrFlag = 0u;


// UART stuff
char txData[TX_DATA_SIZE];

// CAN stuff
CANPacket can_receive;
CANPacket can_send;
uint8 address = 0;

const uint8_t* data;

CY_ISR(Period_Reset_Handler) {
    CAN_time_LED++;
    ERROR_time_LED++;

    if (ERROR_time_LED >= 3) {
        LED_ERR_Write(OFF);
    }
    if (CAN_time_LED >= 3) {
        LED_CAN_Write(OFF);
    }
}

CY_ISR(Button_1_Handler) {
    LED_DBG_Write(!LED_DBG_Read());
}

int32_t pwm_set = 0;

int main(void)
{ 
    Initialize();
    int err;
    
    int16 output;
    uint16 resMilliVolts;
    char8 txData[TX_DATA_SIZE];

    DBG_UART_Start();
    ADC_Pot_Start();
    PWM_Motor_Start();

    /* Start ADC conversion */
    ADC_Pot_StartConvert();

    CAN_Start();

    /* Set CAN interrupt handler to local routine */
    CyIntSetVector(CAN_ISR_NUMBER, ISR_CAN);
    
    CyGlobalIntEnable; /* Enable global interrupts. */

    /* Place your initialization/startup code here (e.g. MyInst_Start()) */
    
    for(;;)
    {
        
        err = 0;
        switch(GetState()) {
            case(UNINIT):
                SetStateTo(CHECK_CAN);
                break;
            case(CHECK_CAN):
                if (!PollAndReceiveCANPacket(&can_receive)) {
                    LED_CAN_Write(ON);
                    CAN_time_LED = 0;
                    err = ProcessCAN(&can_receive, &can_send);
                }
                break;
            case DO_PWM_MODE:
                pwm_set = GetPWMFromPacket(&can_receive);
                PWM_Motor_WriteCompare(pwm_set);
                break;
            case DO_LASER_MODE:
                // mode 1 tasks
                pwm_set = GetPWMFromPacket(&can_receive);
                switch (GetLaserIDFromPacket(&can_receive)) {
                    case 0:
                        PWM_Laser_A_WriteCompare(pwm_set);
                        break;
                    case 1:
                        PWM_Laser_B_WriteCompare(pwm_set);
                        break;    
                }
                
                DBG_UART_UartPutString("PWM Set");
                
                SetStateTo(CHECK_CAN);
                break;
            default:
                err = ERROR_INVALID_STATE;
                SetStateTo(UNINIT);
                break;
        }    
    
        if (err) DisplayErrorCode(err);
        
        if (DBG_UART_SpiUartGetRxBufferSize()) {
            DebugPrint(DBG_UART_UartGetByte());
        }
        
        CyDelay(100);
        
        
        /* Place your application code here. */
        if (ADC_Pot_IsEndConversion(ADC_Pot_RETURN_STATUS))
        {
            /* Gets ADC conversion result */
            output = ADC_Pot_GetResult16(ADC_CHANNEL_NUMBER_0);

            /* Saturates ADC result to positive numbers */
            if (output < 0)
            {
                output = 0;
            }
            
            /* Converts ADC result to milli volts */
            resMilliVolts = (uint16) ADC_Pot_CountsTo_mVolts(ADC_CHANNEL_NUMBER_0, output);
            
            /*
            Unclear what commented code is for.
            */  
            /*
            // Sends value of ADC output via CAN
            CAN_TX_DATA_BYTE1(CAN_TX_MAILBOX_ADCdata, HI8(resMilliVolts));
            CAN_TX_DATA_BYTE2(CAN_TX_MAILBOX_ADCdata, LO8(resMilliVolts));
            CAN_TX_DATA_BYTE3(CAN_TX_MAILBOX_ADCdata, 0u);
            CAN_TX_DATA_BYTE4(CAN_TX_MAILBOX_ADCdata, 0u);
            CAN_TX_DATA_BYTE5(CAN_TX_MAILBOX_ADCdata, 0u);
            CAN_TX_DATA_BYTE6(CAN_TX_MAILBOX_ADCdata, 0u);
            CAN_TX_DATA_BYTE7(CAN_TX_MAILBOX_ADCdata, 0u);
            CAN_TX_DATA_BYTE8(CAN_TX_MAILBOX_ADCdata, 0u);
            CAN_SendMsgADCdata();

            // Display value of ADC output on LCD
            sprintf(txData, "ADC out: %u.%.3u \r\n", (resMilliVolts / 1000u), (resMilliVolts % 1000u));
            DBG_UART_UartPutString(txData);
            */
        }

        /*
            Unclear what commented code is for.
        */
        /*
        // Change configuration at switch press or release event 
        if (switchState != ReadSwSwitch())    // Switch state changed status
        {
            // Store the current switch state
            switchState = ReadSwSwitch();

            // Fill CAN data depending on switch state
            if (Switch1_Read() == 0u)
            {
                CAN_TX_DATA_BYTE1(CAN_TX_MAILBOX_switchStatus, SWITCH_PRESSED);
                DBG_UART_UartPutString("switch1 pressed.\r\n");
            }
            else
            {
                CAN_TX_DATA_BYTE1(CAN_TX_MAILBOX_switchStatus, SWITCH_RELEASED);
                DBG_UART_UartPutString("switch1 released.\r\n");
            }
            CAN_TX_DATA_BYTE2(CAN_TX_MAILBOX_switchStatus, 0u);
            CAN_TX_DATA_BYTE3(CAN_TX_MAILBOX_switchStatus, 0u);
            CAN_TX_DATA_BYTE4(CAN_TX_MAILBOX_switchStatus, 0u);
            CAN_TX_DATA_BYTE5(CAN_TX_MAILBOX_switchStatus, 0u);
            CAN_TX_DATA_BYTE6(CAN_TX_MAILBOX_switchStatus, 0u);
            CAN_TX_DATA_BYTE7(CAN_TX_MAILBOX_switchStatus, 0u);
            CAN_TX_DATA_BYTE8(CAN_TX_MAILBOX_switchStatus, 0u);

            // Send CAN message with switch state
            CAN_SendMsgswitchStatus();
        }
        
        if (isrFlag != 0u)
        {
            // Set PWM pulse width
            PWM_WriteCompare(CAN_RX_DATA_BYTE1(CAN_RX_MAILBOX_0));

            // Puts out over UART value of PWM pulse width
            sprintf(txData, "PWM pulse width: %X \r\n", CAN_RX_DATA_BYTE1(CAN_RX_MAILBOX_0));
            DBG_UART_UartPutString(txData);
            
            // Clear the isrFlag
            isrFlag = 0u;
        }
        */

        CyDelay(100u);
    }
}

void Initialize(void) {
    CyGlobalIntEnable; /* Enable global interrupts. LED arrays need this first */
    
    address = getSerialAddress();
    
    DBG_UART_Start();
    sprintf(txData, "Dip Addr: %x \r\n", address);
    Print(txData);
    
    LED_DBG_Write(0);
    
    InitCAN(0x4, (int)address);
    Timer_Period_Reset_Start();
    
    PWM_Laser_A_Start();
    PWM_Laser_B_Start();
    PWM_Motor_Start();
    
    isr_Period_Reset_StartEx(Period_Reset_Handler);
}

void DebugPrint(char input) {
    switch(input) {
        case 'f':
            sprintf(txData, "Mode: %x State:%x \r\n", GetMode(), GetState());
            break;
        case 'x':
            sprintf(txData, "bruh\r\n");
            break;
        default:
            sprintf(txData, "what\r\n");
            break;
    }
    Print(txData);
}

int getSerialAddress() {
    int address = 0;
    
    if (DIP1_Read()==0) address += 1;
    if (DIP2_Read()==0) address += 2;
    if (DIP3_Read()==0) address += 4;
    if (DIP4_Read()==0) address += 8;
    
    if (address == 0)
        address = DEVICE_SERIAL_TELEM_LOCALIZATION;

    return address;
}

void DisplayErrorCode(uint8_t code) {    
    ERROR_time_LED = 0;
    LED_ERR_Write(ON);
    
    sprintf(txData, "Error %X\r\n", code);
    Print(txData);

    switch(code)
    {
        case ERROR_INVALID_TTC:
            Print("Cannot Send That Data Type!\n\r");
            break;
        default:
            //some error
            break;
    }
}
CY_ISR(ISR_CAN)
{
    /* Clear Receive Message flag */
    CAN_INT_SR_REG = CAN_RX_MESSAGE_MASK;

    /* Set the isrFlag */
    isrFlag = 1u;

    /* Acknowledges receipt of new message */
    CAN_RX_ACK_MESSAGE(CAN_RX_MAILBOX_0);
}
static uint32 ReadSwSwitch(void)
{
    uint32 heldDown;
    uint32 sw1Status;
    uint32 sw2Status;

    sw1Status = 0u;  /* Switch is not active */
    sw2Status = 0u;  /* Switch is not active */
    heldDown = 0u;  /* Reset debounce counter */

    /* Wait for debounce period before determining whether the switch is pressed */
    while (Switch1_Read() == SWITCH_PRESSED)
    {
        /* Count debounce period */
        CyDelay(SWITCH_DEBOUNCE_UNIT);
        ++heldDown;

        if (heldDown > SWITCH_DEBOUNCE_PERIOD)
        {
            sw1Status = 1u; /* Switch is pressed */
            break;
        }
    }
    
    while (Switch2_Read() == SWITCH_PRESSED)
    {
        /* Count debounce period */
        CyDelay(SWITCH_DEBOUNCE_UNIT);
        ++heldDown;

        if (heldDown > SWITCH_DEBOUNCE_PERIOD)
        {
            sw2Status = 1u; /* Switch is pressed */
            break;
        }
    }

    return (sw1Status);
    return (sw2Status);
}



/* [] END OF FILE */
