#include <unistd.h>
#include "py/mpconfig.h"
#include "py/mphal.h"
#include "extmod/misc.h"

#include "os/FreeRTOS/FreeRTOS.h"
#include "os/FreeRTOS/task.h"
#include "shared/runtime/interrupt_char.h"

#include "uart.h"

// Receive single character, blocking until one is available.
int mp_hal_stdin_rx_chr(void) {
    for (;;) {
        #if MICROPY_HW_ENABLE_UART_REPL
        int c = uart_rx_c();
        if (c != -1) {
            return c;
        }
        #endif

        #if MICROPY_PY_OS_DUPTERM
        int dupterm_c = mp_os_dupterm_rx_chr();
        if (dupterm_c >= 0) {
            return dupterm_c;
        }
        #endif

        MICROPY_EVENT_POLL_HOOK
    }

}

// Send the string of given length.
mp_uint_t mp_hal_stdout_tx_strn(const char *str, size_t len) {
    mp_uint_t ret = len;
    bool did_write = false;
    #if MICROPY_HW_ENABLE_UART_REPL
    uart_tx_strn(str, len);
    did_write = true;
    #endif

    #if MICROPY_PY_OS_DUPTERM
    int dupterm_res = mp_os_dupterm_tx_strn(str, len);
    if (dupterm_res >= 0) {
        did_write = true;
        ret = MIN((mp_uint_t)dupterm_res, ret);
    }
    #endif
    return did_write ? ret : 0;
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