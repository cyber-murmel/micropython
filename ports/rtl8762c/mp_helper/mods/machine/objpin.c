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

#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "mphalport.h"
#include "modmachine.h"
#include "machine/objpin.h"

#include "peripheral/rtl876x_rcc.h"
#include "peripheral/rtl876x_pinmux.h"
#include "peripheral/rtl876x_gpio.h"
#include "peripheral/rtl876x_nvic.h"

STATIC pin_obj_t *pin_find_named_pin(const mp_obj_dict_t *named_pins, mp_obj_t name) {
    mp_map_t *named_map = mp_obj_dict_get_map((mp_obj_t)named_pins);
    mp_map_elem_t *named_elem = mp_map_lookup(named_map, name, MP_MAP_LOOKUP);
    if (named_elem != NULL && named_elem->value != NULL) {
        return named_elem->value;
    }
    return NULL;
}

pin_obj_t *pin_find(mp_obj_t user_obj) {
    pin_obj_t *pin_obj;

    // if a pin was provided, use it
    if (MP_OBJ_IS_TYPE(user_obj, &pin_type)) {
        pin_obj = user_obj;
        return pin_obj;
    }
    // otherwise see if the pin name matches a cpu pin
    pin_obj = pin_find_named_pin(&pin_board_pins_locals_dict, user_obj);
    if (pin_obj) {
        return pin_obj;
    }

    mp_raise_ValueError(MP_ERROR_TEXT("invalid argument(s) value"));
}

void machine_pins_init(void) {
    RCC_PeriphClockCmd(APBPeriph_GPIO, APBPeriph_GPIO_CLOCK, ENABLE);
}

void machine_pins_deinit(void) {
    RCC_PeriphClockCmd(APBPeriph_GPIO, APBPeriph_GPIO_CLOCK, DISABLE);

}

// pin.init(mode, pull=None, *, value)
STATIC mp_obj_t pin_obj_init_helper(pin_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_mode, ARG_pull, ARG_value, ARG_sw_mode };
    static const mp_arg_t pin_init_args[] = {
        { MP_QSTR_mode, MP_ARG_INT, {.u_int = PAD_OUT_DISABLE}},
        { MP_QSTR_pull, MP_ARG_OBJ, {.u_obj = mp_const_none}},
        { MP_QSTR_value, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
        { MP_QSTR_sw_mode, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false}},
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(pin_init_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(pin_init_args), pin_init_args, args);

    // get the io mode, default is input
    uint mode = args[ARG_mode].u_int;
    self->sw_mode = args[ARG_sw_mode].u_bool;

    // get the pull type, default is pull none
    uint pull = PAD_PULL_NONE;
    if (args[ARG_pull].u_obj != mp_const_none) {
        pull = mp_obj_get_int(args[ARG_pull].u_obj);
    }

    // get the value, default is 0
    int value = 0;
    if (args[ARG_value].u_obj != MP_OBJ_NULL) {
        if (mp_obj_is_true(args[ARG_value].u_obj)) {
            value = 1;
        }
    }

    // config the pin object
    self->mode  = mode;
    self->pull = pull;

    // config pin hardware

    //// configure the pin for gpio, and pull
    PAD_Mode pad_mode = args[ARG_sw_mode].u_bool
        ? PAD_SW_MODE
        : PAD_PINMUX_MODE;
    PAD_Pull_Mode pull_mode = args[ARG_pull].u_int;
    PAD_OUTPUT_ENABLE_Mode output_enable = args[ARG_mode].u_int;
    PAD_OUTPUT_VAL sw_mode_val = value
        ? PAD_OUT_HIGH
        : PAD_OUT_LOW;

    Pad_Config(self->id, pad_mode, PAD_IS_PWRON, pull_mode, output_enable, sw_mode_val);

    //// configure mode
    if (!self->sw_mode) {
        uint32_t gpioBit = self->gpio_bit;
        GPIOMode_TypeDef direction = (PAD_OUT_DISABLE == output_enable) ? GPIO_Mode_IN : GPIO_Mode_OUT;

        GPIO_InitTypeDef GPIO_InitStruct;
        GPIO_StructInit(&GPIO_InitStruct);
        GPIO_InitStruct.GPIO_Pin = gpioBit;
        GPIO_InitStruct.GPIO_Mode = direction;
        GPIO_Init(&GPIO_InitStruct);
        GPIO_WriteBit(gpioBit, value);
        Pinmux_Config(self->id, DWGPIO);
    }

    return mp_const_none;
}

STATIC void pin_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pin_obj_t *self = self_in;
    qstr mode_qstr = MP_QSTR_OUT;
    qstr pull_qstr = PAD_PULL_NONE;

    if (PAD_OUT_DISABLE == self->mode) {
        mode_qstr = MP_QSTR_IN;
    }

    switch(self->pull) {
        case(PAD_PULL_UP): {
            pull_qstr = MP_QSTR_PULL_UP;
        } break;
        case(PAD_PULL_DOWN): {
            pull_qstr = MP_QSTR_PULL_DOWN;
        } break;
        case(PAD_PULL_NONE): // fall through to default
        default: {
            pull_qstr = MP_QSTR_PULL_NONE;
        } break;
    }

    mp_printf(print, "Pin('%q', mode=Pin.%q, pull=Pin.%q)", self->name, mode_qstr, pull_qstr);
}

// constructor(id, ...)
STATIC mp_obj_t pin_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // Run an argument through the mapper and return the result.
    pin_obj_t *pin = (pin_obj_t *)pin_find(args[0]);

    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    pin_obj_init_helper(pin, n_args - 1, args + 1, &kw_args);

    return MP_OBJ_FROM_PTR(pin);
}

// fast method for getting/setting pin value
STATIC mp_obj_t pin_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    const pin_obj_t *self = self_in;
    uint32_t gpioBit = self->gpio_bit;
    if (n_args == 0) {
        // get pin
        if (self->sw_mode) {
            mp_raise_ValueError(MP_ERROR_TEXT("can't read SW pin"));
        }
        else {
            return MP_OBJ_NEW_SMALL_INT(GPIO_ReadInputDataBit(gpioBit));
        }
    } else {
        // set pin
        if (self->sw_mode) {
            Pad_OutputControlValue(self->id, mp_obj_is_true(args[0]));
        }
        else {
            GPIO_WriteBit(gpioBit, mp_obj_is_true(args[0]));
        }
    }
    return mp_const_none;
}

// pin.init(mode, pull)
STATIC mp_obj_t pin_obj_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return pin_obj_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
MP_DEFINE_CONST_FUN_OBJ_KW(pin_init_obj, 1, pin_obj_init);

// pin.value([value])
STATIC mp_obj_t pin_value(size_t n_args, const mp_obj_t *args) {
    return pin_call(args[0], n_args - 1, 0, args + 1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pin_value_obj, 1, 2, pin_value);

// pin.off()
STATIC mp_obj_t pin_off(mp_obj_t self_in) {
    pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint32_t gpioBit = self->gpio_bit;
    GPIO_WriteBit(gpioBit, 0);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pin_off_obj, pin_off);

// pin.on()
STATIC mp_obj_t pin_on(mp_obj_t self_in) {
    pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint32_t gpioBit = self->gpio_bit;
    GPIO_WriteBit(gpioBit, 1);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pin_on_obj, pin_on);

MP_DEFINE_CONST_OBJ_TYPE(
    pin_board_pins_obj_type,
    MP_QSTR_board,
    MP_TYPE_FLAG_NONE,
    // make_new, pin_make_new,
    print, pin_print,
    // call, pin_call,
    // protocol, &pin_pin_p,
    locals_dict, &pin_board_pins_locals_dict
    );

STATIC const mp_rom_map_elem_t pin_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&pin_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&pin_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&pin_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&pin_on_obj) },

    // class attributes
    { MP_OBJ_NEW_QSTR(MP_QSTR_board),      MP_OBJ_FROM_PTR(&pin_board_pins_obj_type) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_IN), MP_ROM_INT(PAD_OUT_DISABLE) },
    { MP_ROM_QSTR(MP_QSTR_OUT), MP_ROM_INT(PAD_OUT_ENABLE) },
    { MP_ROM_QSTR(MP_QSTR_PULL_UP), MP_ROM_INT(PAD_PULL_UP) },
    { MP_ROM_QSTR(MP_QSTR_PULL_DOWN), MP_ROM_INT(PAD_PULL_DOWN) },
    { MP_ROM_QSTR(MP_QSTR_PULL_NONE), MP_ROM_INT(PAD_PULL_NONE) },
};

STATIC MP_DEFINE_CONST_DICT(pin_locals_dict, pin_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    pin_type,
    MP_QSTR_Pin,
    MP_TYPE_FLAG_NONE,
    make_new, pin_make_new,
    print, pin_print,
    call, pin_call,
    // protocol, &pin_pin_p,
    locals_dict, &pin_locals_dict
    );
