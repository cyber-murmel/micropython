/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2023 Damien P. George
 * Copyright (c) 2016 Paul Sokolovsky
 * Copyright (c) 2016 Linaro Limited
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// This file is never compiled standalone, it's included directly from
// extmod/modmachine.c via MICROPY_PY_MACHINE_INCLUDEFILE.

#include <stdio.h>
#include <zephyr/sys/reboot.h>

#include "modmachine.h"

#ifdef CONFIG_REBOOT
#define MICROPY_PY_MACHINE_RESET_ENTRY { MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&machine_reset_obj) },
#else
#define MICROPY_PY_MACHINE_RESET_ENTRY
#endif

#define MICROPY_PY_MACHINE_EXTRA_GLOBALS \
    MICROPY_PY_MACHINE_RESET_ENTRY \
    { MP_ROM_QSTR(MP_QSTR_reset_cause), MP_ROM_PTR(&machine_reset_cause_obj) }, \
    { MP_ROM_QSTR(MP_QSTR_Pin), MP_ROM_PTR(&machine_pin_type) }, \
    { MP_ROM_QSTR(MP_QSTR_Timer), MP_ROM_PTR(&machine_timer_type) }, \
    { MP_ROM_QSTR(MP_QSTR_unique_id), MP_ROM_PTR(&machine_unique_id_obj) }, \

static mp_obj_t machine_reset(void) {
    sys_reboot(SYS_REBOOT_COLD);
    // Won't get here, Zephyr has infiniloop on its side
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_reset_obj, machine_reset);

static mp_obj_t machine_reset_cause(void) {
    printf("Warning: %s is not implemented\n", __func__);
    return MP_OBJ_NEW_SMALL_INT(42);
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_reset_cause_obj, machine_reset_cause);

static void mp_machine_idle(void) {
    k_yield();
}

#if MICROPY_ESP32_USE_BOOTLOADER_RTC
#if !CONFIG_IDF_TARGET_ESP32P4
#include "soc/rtc_cntl_reg.h"
#include "usb.h"
#include "esp_system.h"
#else
#include "soc/lp_system_reg.h"
#endif
#if CONFIG_IDF_TARGET_ESP32S3
#include "esp32s3/rom/usb/usb_dc.h"
#include "esp32s3/rom/usb/usb_persist.h"
#include "esp32s3/rom/usb/chip_usb_dw_wrapper.h"
#endif

MP_NORETURN void machine_bootloader_rtc(void) {
    #if CONFIG_IDF_TARGET_ESP32S3 && MICROPY_HW_USB_CDC
    usb_usj_mode();
    usb_dc_prepare_persist();
    chip_usb_set_persist_flags(USBDC_BOOT_DFU);
    #endif
    #if !CONFIG_IDF_TARGET_ESP32P4
    REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
    esp_restart();
    #else
    REG_WRITE(LP_SYSTEM_REG_SYS_CTRL_REG, LP_SYSTEM_REG_FORCE_DOWNLOAD_BOOT);
    esp_restart();
    #endif
}
#endif

#ifdef MICROPY_BOARD_ENTER_BOOTLOADER
MP_NORETURN void mp_machine_bootloader(size_t n_args, const mp_obj_t *args) {
    MICROPY_BOARD_ENTER_BOOTLOADER(n_args, args);
    for (;;) {
    }
}
#endif

#if CONFIG_IDF_TARGET_ARCH_XTENSA
#include "esp_mac.h"
static mp_obj_t machine_unique_id(void) {
    uint8_t chipid[6];
    esp_efuse_mac_get_default(chipid);
    return mp_obj_new_bytes(chipid, 6);
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_unique_id_obj, machine_unique_id);
#endif
