all:
	@echo "Targets:"
	@grep "^[\.\w]*:" -P Makefile --color=yes

firmware.hex: USB_MASS.elf
	avr-objcopy -R .eeprom -O ihex $< $@
	cp $@ storage-firmware-`git describe --long`.hex
	ls -l $@ storage-firmware-`git describe --long`.hex

.PHONY: flash
flash: firmware.hex
	sudo dfu-programmer at32uc3a3256s erase
	sleep 1
	sudo dfu-programmer at32uc3a3256s flash --suppress-bootloader-mem firmware.hex
	sleep 1
	-sudo dfu-programmer at32uc3a3256s launch 
	sleep 1
	lsusb | grep 20a0:

