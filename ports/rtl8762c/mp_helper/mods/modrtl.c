/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Paul Sokolovsky
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

#include <stdio.h>

#include "py/gc.h"
#include "py/runtime.h"
#include "py/persistentcode.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "uart.h"
#include "modmachine.h"

#include "drivers/dht/dht.h"
#include "platform/flash_device.h"
#include "config/flash_map.h"

#define MODRTL_INCLUDE_CONSTANTS (1)

#define SECTOR_SIZE_BYTES (FMC_SEC_SECTION_LEN)

static mp_obj_t rtl_flash_read(mp_obj_t offset_in, mp_obj_t len_or_buf_in) {
    mp_int_t offset = mp_obj_get_int(offset_in);

    mp_int_t len;
    byte *buf;
    bool alloc_buf = mp_obj_is_int(len_or_buf_in);

    if (alloc_buf) {
        len = mp_obj_get_int(len_or_buf_in);
        buf = m_new(byte, len);
    } else {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(len_or_buf_in, &bufinfo, MP_BUFFER_WRITE);
        len = bufinfo.len;
        buf = bufinfo.buf;
    }
    
    if (flash_read_locked(offset, len, (uint8_t*)buf)) {
        if (alloc_buf) {
            return mp_obj_new_bytes(buf, len);
        }
        return mp_const_none;
    }
    if (alloc_buf) {
        m_del(byte, buf, len);
    }
    mp_raise_OSError(MP_EIO);
}
static MP_DEFINE_CONST_FUN_OBJ_2(rtl_flash_read_obj, rtl_flash_read);

static mp_obj_t rtl_flash_write(mp_obj_t offset_in, const mp_obj_t buf_in) {
    mp_int_t offset = mp_obj_get_int(offset_in);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);

    // if (flash_write_locked(offset, bufinfo.len, (uint8_t*)bufinfo.buf)) {
    if (flash_auto_write_buffer_locked(offset, (uint32_t*)bufinfo.buf, bufinfo.len)) {
        return mp_const_none;
    }
    mp_raise_OSError(MP_EIO);
}
static MP_DEFINE_CONST_FUN_OBJ_2(rtl_flash_write_obj, rtl_flash_write);

static mp_obj_t rtl_flash_erase(mp_obj_t sector_in) {
    mp_int_t sector = mp_obj_get_int(sector_in);
#if SECTOR_SIZE_BYTES == FMC_SEC_SECTION_LEN
    if (flash_erase_locked(FLASH_ERASE_SECTOR, sector*SECTOR_SIZE_BYTES)) {
#elif SECTOR_SIZE_BYTES == FMC_BLK_SECTION_LEN
    if (flash_erase_locked(FLASH_ERASE_BLOCK, sector*SECTOR_SIZE_BYTES)) {
#endif
        return mp_const_none;
    }
    mp_raise_OSError(MP_EIO);
}
static MP_DEFINE_CONST_FUN_OBJ_1(rtl_flash_erase_obj, rtl_flash_erase);


static const mp_rom_map_elem_t rtl_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_rtl) },

    { MP_ROM_QSTR(MP_QSTR_flash_read), MP_ROM_PTR(&rtl_flash_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_write), MP_ROM_PTR(&rtl_flash_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_erase), MP_ROM_PTR(&rtl_flash_erase_obj) },

    #if MODRTL_INCLUDE_CONSTANTS
    { MP_ROM_QSTR(MP_QSTR_flash_size), MP_ROM_INT(BKP_DATA1_SIZE) },
    { MP_ROM_QSTR(MP_QSTR_flash_user_start), MP_ROM_INT(BKP_DATA1_ADDR) },
    { MP_ROM_QSTR(MP_QSTR_SEC_SIZE), MP_ROM_INT(SECTOR_SIZE_BYTES) },
    #endif
};

static MP_DEFINE_CONST_DICT(rtl_module_globals, rtl_module_globals_table);

const mp_obj_module_t rtl_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&rtl_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_rtl, rtl_module);
