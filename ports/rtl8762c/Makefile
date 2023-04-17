# Include the core environment definitions; this will set $(TOP).
include ../../py/mkenv.mk

# MicroPython feature configurations
MICROPY_ROM_TEXT_COMPRESSION ?= 1

# Include py core make definitions.
include $(TOP)/py/py.mk
include $(TOP)/extmod/extmod.mk
RTLTOOL ?= ~/repos/rtltool/rtltool.py
BOARD ?= EMB1082
BOARD_DIR ?= boards/$(BOARD)

ifeq ($(wildcard $(BOARD_DIR)/.),)
$(error Invalid BOARD specified: $(BOARD))
endif

# Include board specific definitions.
include $(BOARD_DIR)/board_config.mk

CROSS_COMPILE ?= arm-none-eabi-

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
CFLAGS += $(INC) -Wall -Werror -Wdouble-promotion -Wfloat-conversion -std=gnu17 $(COPT) -DFIRMWARE_SIZE=$(FIRMWARE_SIZE)
CFLAGS += -mthumb -mtune=cortex-m4 -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fsingle-precision-constant
LDFLAGS += -specs=nano.specs -T rtl8762c.ld -Wl,-Map=$@.map,--cref -Wl,--gc-sections -Wl,--defsym=FIRMWARE_SIZE=$(FIRMWARE_SIZE) $(CFLAGS)
LIBS += -lc -lm -lnosys sdk/bin/rom_symbol_gcc.axf $(wildcard ./sdk/bin/*.a)
SRC_S = sdk/src/mcu/rtl876x/arm/startup_rtl8762c_gcc.s

# Tune for Debugging or Optimization
ifeq ($(DEBUG), 1)
CFLAGS += -O0 -ggdb -gdwarf-2
else
CFLAGS += -Os -DNDEBUG -fdata-sections -ffunction-sections
endif

# Define the required source files.
SRC_C += \
	main.c \
	uart.c \
	mphalport.c \
	shared/readline/readline.c \
	shared/runtime/gchelper_generic.c \
	shared/runtime/pyexec.c \
	shared/runtime/stdout_helpers.c \
	sdk/src/mcu/rtl876x/system_rtl8762c.c \
	sdk/src/mcu/peripheral/rtl876x_rcc.c \
	sdk/src/mcu/peripheral/rtl876x_uart.c \

SRC_MOD += \
	modmachine.c \

SRC_C += $(SRC_MOD)

SRC_QSTR += \
	$(SRC_MOD) \
	shared/readline/readline.c \
	shared/runtime/pyexec.c

# Define the required object files.
OBJ = \
	$(PY_CORE_O) \
	$(addprefix $(BUILD)/, \
		$(SRC_S:.s=.o) \
		$(SRC_C:.c=.o) \
	)

# Define the top-level target, the main firmware.
all: $(BUILD)/firmware.elf $(BUILD)/firmware.bin $(BUILD)/oem_config.bin $(BUILD)/ota_bank0_header.bin
.PHONY: all

# Define how to build the firmware.
$(BUILD)/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(Q)$(SIZE) $@

$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
	$(Q)$(OBJCOPY) --output-target=binary -R App.trace $^ $@

$(BUILD)/oem_config.o: config/oem_config.c | $(BUILD)
	$(Q)$(CC) -c -Wno-override-init $(CFLAGS) $< -o $@

$(BUILD)/oem_config.bin: $(BUILD)/oem_config.o
	$(Q)$(OBJCOPY) --output-target=binary $< $@
	$(Q)$(PYTHON) tools/crc_patch.py $@

$(BUILD)/ota_bank0_header.o: config/ota_bank0_header.c | $(BUILD)
	$(Q)$(CC) -c -Wno-override-init $(CFLAGS) $< -o $@

$(BUILD)/ota_bank0_header.bin: $(BUILD)/ota_bank0_header.o
	$(Q)$(OBJCOPY) --output-target=binary $< $@
	$(Q)$(PYTHON) tools/crc_patch.py $@

$(BUILD):
	$(MKDIR) -p $@

# Include image creation rules.
include mkimage.mk

deploy: $(BUILD)/oem_config.bin $(BUILD)/ota_bank0_header.bin $(BUILD)/firmware.bin
	$(ECHO) "Writing $^ to the board"
	$(Q)$(PYTHON) $(RTLTOOL) --port $(PORT) write_flash \
		0x00801000 $(BUILD)/oem_config.bin \
		0x00802000 $(BUILD)/ota_bank0_header.bin \
		0x0080E000 $(BUILD)/firmware.bin
.PHONY: deploy

erase:
	$(ECHO) "Erasing firmware"
	$(Q)$(PYTHON) $(RTLTOOL) --port $(PORT) erase_region 0x0080E000 $(FIRMWARE_SIZE)
.PHONY: erase

debug: $(BUILD)/firmware.elf
	$(Q)$(GDB) $< \
		$(addprefix --directory ,$(DIRS)) \
		-ex "target extended-remote | openocd -c 'log_output openocd.log' \
			-f interface/cmsis-dap.cfg \
			-c 'transport select swd' \
			-f debug/rtl8762c.cfg \
			-f debug/gdb.cfg" \
		-ex 'monitor halt'
.PHONY: debug

# Run emulation build on a POSIX system with suitable terminal settings
run: $(BUILD)/firmware.elf
	@saved_=`stty -g`; \
	  stty raw opost -echo; \
	  $<; \
	  echo "Exit status: $$?"; \
	  stty $$saved_
.PHONY: run

# Include remaining core make rules.
include $(TOP)/py/mkrules.mk
