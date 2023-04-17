#ifndef INCLUDED_MPHALPORT_H
#define INCLUDED_MPHALPORT_H

#include "platform/platform_utils.h"
#include "extmod/utime_mphal.h"

static inline void mp_hal_set_interrupt_char(char c) {}

static inline mp_uint_t mp_hal_ticks_cpu(void) {
    return platform_vendor_tick();
}

static inline mp_uint_t mp_hal_ticks_us(void) {
    return platform_vendor_tick()*(CLOCK_40MHZ/1000000u);
}

static inline mp_uint_t mp_hal_ticks_ms(void) {
    return platform_vendor_tick()*(CLOCK_40MHZ/1000);
}

static inline void mp_hal_delay_ms(mp_uint_t time_ms) {
    platform_delay_ms(time_ms);
}

static inline void mp_hal_delay_us(mp_uint_t time_us) {
    platform_delay_us(time_us);
}

#endif // INCLUDED_MPHALPORT_H
