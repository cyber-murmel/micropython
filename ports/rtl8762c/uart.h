#ifndef MICROPY_INCLUDED_RTL8762C_UART_H
#define MICROPY_INCLUDED_RTL8762C_UART_H

void uart_init();
int uart_rx_c(void);
void uart_tx_strn(const char *str, uint32_t len);

#endif//MICROPY_INCLUDED_RTL8762C_UART_H
