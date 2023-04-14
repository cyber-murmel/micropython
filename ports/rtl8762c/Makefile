# Include the core environment definitions; this will set $(TOP).
include ../../py/mkenv.mk

# MicroPython feature configurations
MICROPY_ROM_TEXT_COMPRESSION ?= 1

# Include py core make definitions.
include $(TOP)/py/py.mk
include $(TOP)/extmod/extmod.mk

ifeq ($(CROSS), 1)
CROSS_COMPILE ?= arm-none-eabi-
endif

INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)
INC += -Iconfig
INC += $(addprefix -I,$(sort \
	sdk/inc/ \
	sdk/inc/platform/ \
	sdk/inc/platform/cmsis/ \
	sdk/inc/peripheral/ \
	sdk/inc/os/ \
	sdk/inc/app/ \
))

# Set CFLAGS and libraries.
CFLAGS += $(INC) -Wall -Werror -Wdouble-promotion -Wfloat-conversion -std=gnu17 $(COPT)
LIBS = -lc -lm

ifeq ($(CROSS), 1)
CFLAGS += -mthumb -mtune=cortex-m4 -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fsingle-precision-constant
LDFLAGS += -specs=nano.specs -T rtl8762c.ld -Wl,-Map=$@.map,--cref -Wl,--gc-sections $(CFLAGS)
LIBS += -lnosys sdk/bin/rom_symbol_gcc.axf $(wildcard ./sdk/bin/*.a)
SRC_S = sdk/src/mcu/rtl876x/arm/startup_rtl8762c_gcc.s
SRC_C = sdk/src/mcu/rtl876x/system_rtl8762c.c
else
LD = $(CC)
LDFLAGS += -Wl,-Map=$@.map,--cref -Wl,--gc-sections
endif

# Tune for Debugging or Optimization
ifeq ($(DEBUG), 1)
CFLAGS += -O0 -ggdb -gdwarf-2
else
CFLAGS += -Os -DNDEBUG -fdata-sections -ffunction-sections
endif

# Define the required source files.
SRC_C += \
	main.c \
	mphalport.c \
	shared/readline/readline.c \
	shared/runtime/gchelper_generic.c \
	shared/runtime/pyexec.c \
	shared/runtime/stdout_helpers.c \

SRC_QSTR += shared/readline/readline.c shared/runtime/pyexec.c

# Define the required object files.
OBJ = \
	$(PY_CORE_O) \
	$(addprefix $(BUILD)/, \
		$(SRC_S:.s=.o) \
		$(SRC_C:.c=.o) \
	)

# Define the top-level target, the main firmware.
all: $(BUILD)/firmware.elf

# Define how to build the firmware.
$(BUILD)/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(Q)$(SIZE) $@

# Include remaining core make rules.
include $(TOP)/py/mkrules.mk
