#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "mphalport.h"
#include "modmachine.h"

#include "peripheral/rtl876x_rcc.h"
#include "peripheral/rtl876x_pinmux.h"
#include "peripheral/rtl876x_gpio.h"
#include "peripheral/rtl876x_nvic.h"

typedef struct _machine_pin_obj_t {
    const mp_obj_base_t base;
    const uint8_t id;
    const uint32_t gpioBit;
    bool sw_mode;
} machine_pin_obj_t;

#define IRQn_hlp(PAD) GPIO##PAD##_IRQn
#define IRQn(PAD) IRQn_hlp(PAD)

STATIC machine_pin_obj_t machine_pin_obj[] = {
    {{&machine_pin_type}, P0_0, IRQn(P0_0), false},
    {{&machine_pin_type}, P0_1, IRQn(P0_1), false},
    {{&machine_pin_type}, P0_2, IRQn(P0_2), false},
    {{&machine_pin_type}, P0_3, IRQn(P0_3), false},
    {{&machine_pin_type}, P0_4, IRQn(P0_4), false},
    {{&machine_pin_type}, P0_5, IRQn(P0_5), false},
    {{&machine_pin_type}, P0_6, IRQn(P0_6), false},
    {{&machine_pin_type}, P0_7, IRQn(P0_7), false},
    {{&machine_pin_type}, P1_0, IRQn(P1_0), false},
    {{&machine_pin_type}, P1_1, IRQn(P1_1), false},
    {{&machine_pin_type}, P1_2, IRQn(P1_2), false},
    {{&machine_pin_type}, P1_3, IRQn(P1_3), false},
    {{&machine_pin_type}, P1_4, IRQn(P1_4), false},
    {{&machine_pin_type}, P1_5, IRQn(P1_5), false},
    {{&machine_pin_type}, P1_6, IRQn(P1_6), false},
    {{&machine_pin_type}, P1_7, IRQn(P1_7), false},
    {{&machine_pin_type}, P2_0, IRQn(P2_0), false},
    {{&machine_pin_type}, P2_1, IRQn(P2_1), false},
    {{&machine_pin_type}, P2_2, IRQn(P2_2), false},
    {{&machine_pin_type}, P2_3, IRQn(P2_3), false},
    {{&machine_pin_type}, P2_4, IRQn(P2_4), false},
    {{&machine_pin_type}, P2_5, IRQn(P2_5), false},
    {{&machine_pin_type}, P2_6, IRQn(P2_6), false},
    {{&machine_pin_type}, P2_7, IRQn(P2_7), false},
    {{&machine_pin_type}, P3_0, IRQn(P3_0), false},    // U0TXD
    {{&machine_pin_type}, P3_1, IRQn(P3_1), false},    // U0RXD
    {{&machine_pin_type}, P3_2, IRQn(P3_2), false},
    {{&machine_pin_type}, P3_3, IRQn(P3_3), false},
    {{&machine_pin_type}, P3_4, IRQn(P3_4), false},
    {{&machine_pin_type}, P3_5, IRQn(P3_5), false},
    {{&machine_pin_type}, P3_6, IRQn(P3_6), false},
    {{NULL}},
    {{&machine_pin_type}, P4_0, IRQn(P3_3), false},
    {{&machine_pin_type}, P4_1, IRQn(P3_4), false},
    {{&machine_pin_type}, P4_2, IRQn(P3_5), false},
    {{&machine_pin_type}, P4_3, IRQn(P3_6), false},
    {{&machine_pin_type}, H_0, IRQn(P3_0), false},     // MICBIAS
    {{&machine_pin_type}, H_1, IRQn(P3_1), false},     // 32K_XI
    {{&machine_pin_type}, H_2, IRQn(P3_2), false},     // 32K_XO
};

void machine_pins_init(void) {
    RCC_PeriphClockCmd(APBPeriph_GPIO, APBPeriph_GPIO_CLOCK, ENABLE);
}

void machine_pins_deinit(void) {
    RCC_PeriphClockCmd(APBPeriph_GPIO, APBPeriph_GPIO_CLOCK, DISABLE);

}

STATIC void machine_pin_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_pin_obj_t *self = self_in;
    mp_printf(print, "Pin(%u)", self->id);
}

// pin.init(mode, pull=None, *, value)
STATIC mp_obj_t machine_pin_obj_init_helper(machine_pin_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_mode, ARG_pull, ARG_sw_mode, ARG_value };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode, MP_ARG_INT, {.u_int = PAD_OUT_DISABLE}},
        { MP_QSTR_pull, MP_ARG_INT, {.u_int = PAD_PULL_NONE}},
        { MP_QSTR_sw_mode, MP_ARG_BOOL, {.u_bool = false}},
        { MP_QSTR_value, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false}},
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    self->sw_mode = args[ARG_sw_mode].u_bool;

    // configure the pin for gpio, and pull
    PAD_Mode pad_mode = args[ARG_sw_mode].u_bool
        ? PAD_SW_MODE
        : PAD_PINMUX_MODE;
    PAD_Pull_Mode pull_mode = args[ARG_pull].u_int;
    PAD_OUTPUT_ENABLE_Mode output_enable = args[ARG_mode].u_int;
    PAD_OUTPUT_VAL sw_mode_val = args[ARG_value].u_bool
        ? PAD_OUT_HIGH
        : PAD_OUT_LOW;

    Pad_Config(self->id, pad_mode, PAD_IS_PWRON, pull_mode, output_enable, sw_mode_val);

    // configure mode
    if (!self->sw_mode) {
        uint32_t gpioBit = GPIO_GetPin(self->id);
        GPIOMode_TypeDef direction = (PAD_OUT_DISABLE == output_enable) ? GPIO_Mode_IN : GPIO_Mode_OUT;

        GPIO_InitTypeDef GPIO_InitStruct;
        GPIO_StructInit(&GPIO_InitStruct);
        GPIO_InitStruct.GPIO_Pin = gpioBit;
        GPIO_InitStruct.GPIO_Mode = direction;
        GPIO_Init(&GPIO_InitStruct);
        GPIO_WriteBit(gpioBit, args[ARG_value].u_bool);
        Pinmux_Config(self->id, DWGPIO);
    }

    return mp_const_none;
}

// constructor(id, ...)
mp_obj_t mp_pin_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // get the wanted pin object
    int wanted_pin = mp_obj_get_int(args[0]);
    machine_pin_obj_t *self = NULL;
    if (0 <= wanted_pin && wanted_pin < MP_ARRAY_SIZE(machine_pin_obj)) {
        self = (machine_pin_obj_t *)&machine_pin_obj[wanted_pin];
    }
    if (self == NULL || self->base.type == NULL) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid pin"));
    }

    if (n_args > 1 || n_kw > 0) {
        // pin mode given, so configure this GPIO
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
        machine_pin_obj_init_helper(self, n_args - 1, args + 1, &kw_args);
    }

    return MP_OBJ_FROM_PTR(self);
}

// fast method for getting/setting pin value
STATIC mp_obj_t machine_pin_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    const machine_pin_obj_t *self = self_in;
    uint32_t gpioBit = GPIO_GetPin(self->id);
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
STATIC mp_obj_t machine_pin_obj_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return machine_pin_obj_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_pin_init_obj, 1, machine_pin_obj_init);

// pin.value([value])
STATIC mp_obj_t machine_pin_value(size_t n_args, const mp_obj_t *args) {
    return machine_pin_call(args[0], n_args - 1, 0, args + 1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pin_value_obj, 1, 2, machine_pin_value);

// pin.off()
STATIC mp_obj_t machine_pin_off(mp_obj_t self_in) {
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint32_t gpioBit = GPIO_GetPin(self->id);
    GPIO_WriteBit(gpioBit, 0);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_off_obj, machine_pin_off);

// pin.on()
STATIC mp_obj_t machine_pin_on(mp_obj_t self_in) {
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint32_t gpioBit = GPIO_GetPin(self->id);
    GPIO_WriteBit(gpioBit, 1);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_on_obj, machine_pin_on);

STATIC const mp_rom_map_elem_t machine_pin_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_pin_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&machine_pin_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&machine_pin_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&machine_pin_on_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_IN), MP_ROM_INT(PAD_OUT_DISABLE) },
    { MP_ROM_QSTR(MP_QSTR_OUT), MP_ROM_INT(PAD_OUT_ENABLE) },
    { MP_ROM_QSTR(MP_QSTR_PULL_UP), MP_ROM_INT(PAD_PULL_UP) },
    { MP_ROM_QSTR(MP_QSTR_PULL_DOWN), MP_ROM_INT(PAD_PULL_DOWN) },
    { MP_ROM_QSTR(MP_QSTR_PULL_NONE), MP_ROM_INT(PAD_PULL_NONE) },
    { MP_ROM_QSTR(MP_QSTR_P0_0), MP_ROM_INT(P0_0) },
    { MP_ROM_QSTR(MP_QSTR_P0_1), MP_ROM_INT(P0_1) },
    { MP_ROM_QSTR(MP_QSTR_P0_2), MP_ROM_INT(P0_2) },
    { MP_ROM_QSTR(MP_QSTR_P0_3), MP_ROM_INT(P0_3) },
    { MP_ROM_QSTR(MP_QSTR_P0_4), MP_ROM_INT(P0_4) },
    { MP_ROM_QSTR(MP_QSTR_P0_5), MP_ROM_INT(P0_5) },
    { MP_ROM_QSTR(MP_QSTR_P0_6), MP_ROM_INT(P0_6) },
    { MP_ROM_QSTR(MP_QSTR_P0_7), MP_ROM_INT(P0_7) },
    { MP_ROM_QSTR(MP_QSTR_P1_0), MP_ROM_INT(P1_0) },
    { MP_ROM_QSTR(MP_QSTR_P1_1), MP_ROM_INT(P1_1) },
    { MP_ROM_QSTR(MP_QSTR_P1_2), MP_ROM_INT(P1_2) },
    { MP_ROM_QSTR(MP_QSTR_P1_3), MP_ROM_INT(P1_3) },
    { MP_ROM_QSTR(MP_QSTR_P1_4), MP_ROM_INT(P1_4) },
    { MP_ROM_QSTR(MP_QSTR_P1_5), MP_ROM_INT(P1_5) },
    { MP_ROM_QSTR(MP_QSTR_P1_6), MP_ROM_INT(P1_6) },
    { MP_ROM_QSTR(MP_QSTR_P1_7), MP_ROM_INT(P1_7) },
    { MP_ROM_QSTR(MP_QSTR_P2_0), MP_ROM_INT(P2_0) },
    { MP_ROM_QSTR(MP_QSTR_P2_1), MP_ROM_INT(P2_1) },
    { MP_ROM_QSTR(MP_QSTR_P2_2), MP_ROM_INT(P2_2) },
    { MP_ROM_QSTR(MP_QSTR_P2_3), MP_ROM_INT(P2_3) },
    { MP_ROM_QSTR(MP_QSTR_P2_4), MP_ROM_INT(P2_4) },
    { MP_ROM_QSTR(MP_QSTR_P2_5), MP_ROM_INT(P2_5) },
    { MP_ROM_QSTR(MP_QSTR_P2_6), MP_ROM_INT(P2_6) },
    { MP_ROM_QSTR(MP_QSTR_P2_7), MP_ROM_INT(P2_7) },
    { MP_ROM_QSTR(MP_QSTR_P3_0), MP_ROM_INT(P3_0) },
    { MP_ROM_QSTR(MP_QSTR_P3_1), MP_ROM_INT(P3_1) },
    { MP_ROM_QSTR(MP_QSTR_P3_2), MP_ROM_INT(P3_2) },
    { MP_ROM_QSTR(MP_QSTR_P3_3), MP_ROM_INT(P3_3) },
    { MP_ROM_QSTR(MP_QSTR_P3_4), MP_ROM_INT(P3_4) },
    { MP_ROM_QSTR(MP_QSTR_P3_5), MP_ROM_INT(P3_5) },
    { MP_ROM_QSTR(MP_QSTR_P3_6), MP_ROM_INT(P3_6) },
    { MP_ROM_QSTR(MP_QSTR_P4_0), MP_ROM_INT(P4_0) },
    { MP_ROM_QSTR(MP_QSTR_P4_1), MP_ROM_INT(P4_1) },
    { MP_ROM_QSTR(MP_QSTR_P4_2), MP_ROM_INT(P4_2) },
    { MP_ROM_QSTR(MP_QSTR_P4_3), MP_ROM_INT(P4_3) },
    { MP_ROM_QSTR(MP_QSTR_H_0), MP_ROM_INT(H_0) },
    { MP_ROM_QSTR(MP_QSTR_H_1), MP_ROM_INT(H_1) },
    { MP_ROM_QSTR(MP_QSTR_H_2), MP_ROM_INT(H_2) },
};

STATIC MP_DEFINE_CONST_DICT(machine_pin_locals_dict, machine_pin_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_pin_type,
    MP_QSTR_Pin,
    MP_TYPE_FLAG_NONE,
    make_new, mp_pin_make_new,
    print, machine_pin_print,
    call, machine_pin_call,
    // protocol, &pin_pin_p,
    locals_dict, &machine_pin_locals_dict
    );
