FSBL ?= fsbl_MP_master\#\#_1.1.4.0_c6f6dbf5-6d099f4054016ab6d562698d14e662e9.bin
PATCH ?= Patch_MP_release\#_1.0.611.1_130fa89-1c796670a5129533908722a146121972.bin

image: $(BUILD)/image.bin
.PHONY: image

$(BUILD)/image.bin: $(BUILD)/oem_config.bin $(BUILD)/ota_bank0_header.bin $(BUILD)/patch.bin $(BUILD)/fsbl.bin $(BUILD)/firmware.bin
	$(Q)dd bs=1 of=$@     seek=0 if=$(BUILD)/oem_config.bin
	$(Q)dd bs=1 of=$@  seek=4096 if=$(BUILD)/ota_bank0_header.bin
	$(Q)dd bs=1 of=$@  seek=8192 if=$(BUILD)/patch.bin
	$(Q)dd bs=1 of=$@ seek=49152 if=$(BUILD)/fsbl.bin
	$(Q)dd bs=1 of=$@ seek=53248 if=$(BUILD)/firmware.bin

$(BUILD)/patch.bin: $(PATCH) | $(BUILD)
	$(Q)dd if=$< bs=1 skip=512 of=$@

$(BUILD)/fsbl.bin: $(FSBL) | $(BUILD)
	$(Q)dd if=$< bs=1 skip=512 of=$@

deploy_image: $(BUILD)/image.bin
	$(ECHO) "Writing $< to the board"
	$(Q)$(PYTHON) $(RTLTOOL) --port $(PORT) write_flash 0x00801000 $<
.PHONY: deploy_image

erase_board:
	$(ECHO) "Erasing board"
	$(Q)$(PYTHON) $(RTLTOOL) --port $(PORT) erase_flash
.PHONY: erase_board
