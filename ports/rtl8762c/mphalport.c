#include <unistd.h>
#include "py/mpconfig.h"
#include "py/mphal.h"

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
mp_uint_t mp_hal_stdout_tx_strn(const char *str, size_t len) {
    mp_uint_t ret = len;
    uart_tx_strn(str, len);
    return ret;
    // write(STDOUT_FILENO, str, len);
}

mp_uint_t mp_hal_ticks_cpu(void) {
    return platform_vendor_tick();
}

mp_uint_t mp_hal_ticks_us(void) {
    return platform_vendor_tick()*(CLOCK_40MHZ/1000000u);
}

mp_uint_t mp_hal_ticks_ms(void) {
    return platform_vendor_tick()*(CLOCK_40MHZ/1000);
}

void mp_hal_delay_ms(mp_uint_t time_ms) {
    platform_delay_ms(time_ms);
}

void mp_hal_delay_us(mp_uint_t time_us) {
    platform_delay_us(time_us);
}