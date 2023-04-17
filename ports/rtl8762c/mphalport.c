#include <unistd.h>
#include "py/mpconfig.h"

#include "os/FreeRTOS/FreeRTOS.h"
#include "os/FreeRTOS/task.h"

#include "uart.h"

// Receive single character, blocking until one is available.
int mp_hal_stdin_rx_chr(void) {
    for (;;) {
        int c = uart_rx_c();
        if (c != -1) {
            return c;
        }
        c = read(STDIN_FILENO, &c, 1);
        if (c != -1) {
            return c;
        }
        MICROPY_EVENT_POLL_HOOK
    }

}

// Send the string of given length.
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    uart_tx_strn(str, len);
    write(STDOUT_FILENO, str, len);
}
