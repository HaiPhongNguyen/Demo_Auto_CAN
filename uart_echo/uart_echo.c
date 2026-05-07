// ======================================================
// INCLUDE
// ======================================================

#include <stdint.h>
#include <stdbool.h>

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"

#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/can.h"
#include "driverlib/uart.h"

// ======================================================
// CAN
// ======================================================

tCANMsgObject rxMsg;
tCANMsgObject txMsg;

uint8_t rxData[8];
uint8_t txData[8];

// ======================================================
// UART BUFFER
// ======================================================

uint8_t uartBuf[8];
uint8_t uartCount = 0;

// ======================================================
// UART SEND STRING
// ======================================================

void UARTSendString(const char *str)
{
    while(*str)
    {
        UARTCharPut(UART0_BASE, *str++);
    }
}

// ======================================================
// UART SEND NUMBER
// ======================================================

void UARTSendNumber(uint32_t num)
{
    char buf[16];
    int i = 0;

    if(num == 0)
    {
        UARTCharPut(UART0_BASE, '0');
        return;
    }

    while(num > 0)
    {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }

    while(i--)
    {
        UARTCharPut(UART0_BASE, buf[i]);
    }
}

// ======================================================
// UART SEND HEX
// ======================================================

void UARTSendHex(uint32_t num)
{
    char hex[] = "0123456789ABCDEF";
    int i = 0;
    UARTSendString("0x");

    for(i = 12; i >= 0; i -= 4)
    {
        UARTCharPut(UART0_BASE, hex[(num >> i) & 0xF]);
    }
}

// ======================================================
// SETUP UART0
// PA0 = RX
// PA1 = TX
// ======================================================

void UART0_Init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));

    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);

    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTConfigSetExpClk(
        UART0_BASE,
        SysCtlClockGet(),
        115200,
        (UART_CONFIG_WLEN_8 |
         UART_CONFIG_STOP_ONE |
         UART_CONFIG_PAR_NONE)
    );
}

// ======================================================
// SETUP CAN0
// PB4 = RX
// PB5 = TX
// ======================================================

void CAN0_Init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);

    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_CAN0));

    GPIOPinConfigure(GPIO_PB4_CAN0RX);
    GPIOPinConfigure(GPIO_PB5_CAN0TX);

    GPIOPinTypeCAN(GPIO_PORTB_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    CANInit(CAN0_BASE);

    // 100 kbps
    CANBitRateSet(
        CAN0_BASE,
        SysCtlClockGet(),
        100000
    );

    CANEnable(CAN0_BASE);

    // ==================================================
    // RX ALL ID
    // ==================================================

    rxMsg.ui32MsgID = 0;
    rxMsg.ui32MsgIDMask = 0;

    rxMsg.ui32Flags =
            MSG_OBJ_RX_INT_ENABLE |
            MSG_OBJ_USE_ID_FILTER;

    rxMsg.ui32MsgLen = 8;
    rxMsg.pui8MsgData = rxData;

    CANMessageSet(
        CAN0_BASE,
        1,
        &rxMsg,
        MSG_OBJ_TYPE_RX
    );
}

// ======================================================
// CAN -> UART
// ======================================================

void readCAN(void)
{
    uint32_t status;

    status = CANIntStatus(
                CAN0_BASE,
                CAN_INT_STS_CAUSE
             );

    if(status == 1)
    {   
        int i = 0;
        CANMessageGet(
            CAN0_BASE,
            1,
            &rxMsg,
            true
        );

        // SEND ID
        UARTSendNumber(rxMsg.ui32MsgID);

        // SEND DATA
        for(i = 0; i < rxMsg.ui32MsgLen; i++)
        {
            UARTCharPut(UART0_BASE, ',');
            UARTSendNumber(rxData[i]);
        }

        UARTSendString("\r\n");

        CANIntClear(CAN0_BASE, 1);
    }
}

// ======================================================
// UART -> CAN
// FORMAT:
// [ID_H][ID_L][DLC][DATA0][DATA1]
// ======================================================

void readUART(void)
{
    while(UARTCharsAvail(UART0_BASE))
    {
        uartBuf[uartCount++] =
            UARTCharGet(UART0_BASE);

        if(uartCount >= 5)
        {
            uint16_t id;
            uint8_t dlc;
            int i = 3;

            id  = (uartBuf[0] << 8);
            id |= uartBuf[1];

            dlc = uartBuf[2];

            // DEBUG
            UARTSendString("UART RX -> ID: ");
            UARTSendHex(id);

            UARTSendString(" DLC: ");
            UARTSendNumber(dlc);

            UARTSendString(" DATA: ");

            for(i = 3; i < 5; i++)
            {
                UARTSendHex(uartBuf[i]);
                UARTCharPut(UART0_BASE, ' ');
            }

            UARTSendString("\r\n");

            // CAN TX
            txMsg.ui32MsgID = id;
            txMsg.ui32MsgLen = dlc;
            txMsg.ui32Flags = MSG_OBJ_NO_FLAGS;

            txData[0] = uartBuf[3];
            txData[1] = uartBuf[4];

            txMsg.pui8MsgData = txData;

            SysCtlDelay(SysCtlClockGet()/3000);

            CANMessageSet(
                CAN0_BASE,
                2,
                &txMsg,
                MSG_OBJ_TYPE_TX
            );

            SysCtlDelay(SysCtlClockGet()/3000);

            uint32_t st;

            st = CANStatusGet(
                    CAN0_BASE,
                    CAN_STS_CONTROL
                 );

            if(st & (
                CAN_STATUS_BUS_OFF |
                CAN_STATUS_EWARN   |
                CAN_STATUS_EPASS))
            {
                UARTSendString("E\r\n");
            }
            else
            {
                UARTSendString("O\r\n");
            }

            uartCount = 0;
        }

        if(uartCount >= 8)
        {
            uartCount = 0;
        }
    }
}

// ======================================================
// MAIN
// ======================================================

int main(void)
{
    // Clock 16 MHz
    SysCtlClockSet(
        SYSCTL_SYSDIV_1 |
        SYSCTL_USE_OSC  |
        SYSCTL_OSC_MAIN |
        SYSCTL_XTAL_16MHZ
    );

    UART0_Init();

    CAN0_Init();

    UARTSendString("TIVA READY\r\n");

    while(1)
    {
        readCAN();

        readUART();
    }
}