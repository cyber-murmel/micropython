/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Markus "marble" Blechschmidt
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

#include "platform/platform_utils.h"
#include "peripheral/rtl876x_wdg.h"
#include "os/os_task.h"

#include "py/obj.h"
#include "py/runtime.h"
#include "shared/runtime/pyexec.h"

#include "modmachine.h"
#include "machine/objpin.h"

#define MICROPY_PY_MACHINE_EXTRA_GLOBALS \
    { MP_ROM_QSTR(MP_QSTR_Pin), MP_ROM_PTR(&pyb_pin_type) }, \


// #if MICROPY_PY_MACHINE

// static mp_obj_t mp_machine_get_freq(void) {
//     return mp_obj_new_int(CLOCK_40MHZ);
// }
// STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_freq_obj, 0, 1, machine_freq);

// STATIC mp_obj_t machine_reset(void) {
//     WDG_SystemReset(RESET_ALL, RESET_REASON_HW);
//     return mp_const_none;
// }
// STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_reset_obj, machine_reset);

// STATIC mp_obj_t machine_soft_reset(void) {
//     pyexec_system_exit = PYEXEC_FORCED_EXIT;
//     mp_raise_type(&mp_type_SystemExit);
// }
// STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_soft_reset_obj, machine_soft_reset);


// STATIC const mp_rom_map_elem_t machine_module_globals_table[] = {
//     { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_umachine) },

//     { MP_ROM_QSTR(MP_QSTR_freq), MP_ROM_PTR(&machine_freq_obj) },
//     { MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&machine_reset_obj) },
//     { MP_ROM_QSTR(MP_QSTR_soft_reset), MP_ROM_PTR(&machine_soft_reset_obj) },
//     { MP_ROM_QSTR(MP_QSTR_Pin), MP_ROM_PTR(&pin_type) },
// };

// STATIC MP_DEFINE_CONST_DICT(machine_module_globals, machine_module_globals_table);

// const mp_obj_module_t mp_module_machine = {
//     .base = { &mp_type_module },
//     .globals = (mp_obj_dict_t *)&machine_module_globals,
// };

// MP_REGISTER_MODULE(MP_QSTR_umachine, mp_module_machine);

static void mp_machine_idle(void) {
    MP_THREAD_GIL_EXIT();
    os_task_yield();
    MP_THREAD_GIL_ENTER();
}

// #endif // MICROPY_PY_MACHINE
