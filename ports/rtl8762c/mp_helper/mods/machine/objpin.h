/**
 * @file objpin.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-04-25
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef OBJPIN_H_
#define OBJPIN_H_

#include "py/obj.h"

#include "peripheral/rtl876x_pinmux.h"
#include "peripheral/rtl876x_gpio.h"
#include "peripheral/rtl876x_nvic.h"

typedef struct {
    const mp_obj_base_t     base;
    const qstr              name;
    const uint8_t           id;
    const uint32_t          gpio_bit;
    PAD_Pull_Mode           pull;
    PAD_OUTPUT_ENABLE_Mode  mode;
    PAD_OUTPUT_VAL          value;
    const IRQn_Type         irqn;
    bool sw_mode;
} pin_obj_t;

typedef struct {
    size_t num;
    pin_obj_t pins[];
} board_pins_t;

#define IRQn_hlp(PAD) GPIO##PAD##_IRQn
#define IRQn(PAD) IRQn_hlp(PAD)

// Some pads use the same GPIO register bit.
// This should only be relevant regarding input,
// since output can be accomplished with SW mode.
#define GPIO_BIT_hlp(PAD)   (PAD<=P3_6) ? BIT(PAD) : \
                        ((PAD <= P4_3) && (PAD >= P4_0)) ? BIT(PAD - 4) : \
                        ((PAD == H_0) || (PAD == H_1) || (PAD == H_2)) ? BIT(PAD - 11) : \
                        0xff
#define GPIO_BIT(PAD) GPIO_BIT_hlp(PAD)

// Some pads reuse other pads' gpio interrupt.
// For this reason there is a `p_pad_irq` parameter.
#define PIN(p_name, p_pad, p_pad_irq, p_pull, p_mode, p_value) ((pin_obj_t){ \
    {&pin_type}, \
    .name       = p_name, \
    .id         = p_pad, \
    .gpio_bit   = GPIO_BIT(p_pad), \
    .pull       = p_pull, \
    .mode       = p_mode, \
    .value      = p_value, \
    .irqn       = IRQn(p_pad_irq), \
    .sw_mode    = false})

#define BOARD_PIN_hlp(p_name, p_pad, p_pad_irq) ((mp_map_elem_t) { \
    MP_OBJ_NEW_QSTR(p_name), \
    MP_OBJ_FROM_PTR(&PIN(p_name, p_pad, p_pad_irq, PAD_PULL_NONE, PAD_OUT_DISABLE, 0)) \
})
#define BOARD_PIN_REUSE_IRQ(p_pad, p_pad_irq) BOARD_PIN_hlp(MP_QSTR_ ## p_pad, p_pad, p_pad_irq)
#define BOARD_PIN(p_pad) BOARD_PIN_hlp(MP_QSTR_ ## p_pad, p_pad, p_pad)

extern const mp_obj_type_t pin_type;
extern const mp_obj_dict_t pin_board_pins_locals_dict;

#endif  // OBJPIN_H_
