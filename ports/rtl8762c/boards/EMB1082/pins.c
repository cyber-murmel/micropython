#include "objpin.h"

STATIC const mp_map_elem_t pin_board_pins_locals_dict_table[] = {
    BOARD_PIN(P0_0),
    BOARD_PIN(P0_1),
    BOARD_PIN(P0_2),
    BOARD_PIN(P0_3),
    BOARD_PIN(P0_4),
    BOARD_PIN(P0_5),
    BOARD_PIN(P0_6),
    BOARD_PIN(P0_7),
    BOARD_PIN(P1_0),
    BOARD_PIN(P1_1),
    BOARD_PIN(P1_2),
    BOARD_PIN(P1_3),
    BOARD_PIN(P1_4),
    BOARD_PIN(P1_5),
    BOARD_PIN(P1_6),
    BOARD_PIN(P1_7),
    BOARD_PIN(P2_0), // ADC 0
    BOARD_PIN(P2_1), // ADC 1
    BOARD_PIN(P2_2), // ADC 2
    BOARD_PIN(P2_3), // ADC 3
    BOARD_PIN(P2_4), // ADC 4
    BOARD_PIN(P2_5), // ADC 5
    BOARD_PIN(P2_6), // ADC 6
    BOARD_PIN(P2_7), // ADC 7
    BOARD_PIN(P3_0), // U0TXD
    BOARD_PIN(P3_1), // U0TXD
    BOARD_PIN(P3_2),
    BOARD_PIN(P3_3),
    BOARD_PIN(P3_4),
    BOARD_PIN(P3_5),
    BOARD_PIN(P3_6),
    // the following pins reuse other pins' IRQs
    BOARD_PIN_REUSE_IRQ(P4_0, P3_3),
    BOARD_PIN_REUSE_IRQ(P4_1, P3_4),
    BOARD_PIN_REUSE_IRQ(P4_2, P3_5),
    BOARD_PIN_REUSE_IRQ(P4_3, P3_6),
    BOARD_PIN_REUSE_IRQ(H_0, P3_0),  // MICBIAS
    BOARD_PIN_REUSE_IRQ(H_1, P3_1),  // 32K_XI
    BOARD_PIN_REUSE_IRQ(H_2, P3_2),  // 32K_XO

};
MP_DEFINE_CONST_DICT(pin_board_pins_locals_dict, pin_board_pins_locals_dict_table);
