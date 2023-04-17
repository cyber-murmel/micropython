#include <string.h>

#include <os/os_msg.h>
#include <os/os_sync.h>

#include <peripheral/rtl876x_gdma.h>
#include <peripheral/rtl876x_nvic.h>
#include <peripheral/rtl876x_pinmux.h>
#include <peripheral/rtl876x_rcc.h>
#include <peripheral/rtl876x_uart.h>

#include "uart.h"

void uart_init() {
    Pad_Config(P3_0, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP,
        PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(P3_1, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP,
        PAD_OUT_DISABLE, PAD_OUT_HIGH);

    Pinmux_Config(P3_0, UART0_TX);
    Pinmux_Config(P3_1, UART0_RX);

    RCC_PeriphClockCmd(APBPeriph_UART0, APBPeriph_UART0_CLOCK, ENABLE);

    /* uart init */
    UART_InitTypeDef UART_InitStruct;
    UART_StructInit(&UART_InitStruct);

    UART_InitStruct.parity         = UART_PARITY_NO_PARTY;
    UART_InitStruct.stopBits       = UART_STOP_BITS_1;
    UART_InitStruct.wordLen        = UART_WROD_LENGTH_8BIT;
    UART_InitStruct.rxTriggerLevel = 16;                      //1~29
    UART_InitStruct.idle_time      = UART_RX_IDLE_2BYTE;      //idle interrupt wait time

    UART_Init(UART, &UART_InitStruct);
}

char txBuf[128];

void uart_tx_strn(const char *str, uint32_t len) {
    uint8_t count;

    while (len / UART_TX_FIFO_SIZE > 0)
    {
        while (UART_GetFlagState(UART, UART_FLAG_THR_EMPTY) == 0);
        for (count = UART_TX_FIFO_SIZE; count > 0; count--)
        {
            UART->RB_THR = *str++;
        }
        len -= UART_TX_FIFO_SIZE;
    }

    while (UART_GetFlagState(UART, UART_FLAG_THR_EMPTY) == 0);
    while (len--)
    {
        UART->RB_THR = *str++;
    }
}

int uart_rx_c(void) {
    int rx_byte = -1;
    if (UART_GetFlagState(UART, UART_FLAG_RX_DATA_RDY) == SET)
    {
        rx_byte = UART_ReceiveByte(UART);
    }
    return rx_byte;
}

