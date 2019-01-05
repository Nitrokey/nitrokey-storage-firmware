
firmware.hex: USB_MASS.elf
	avr-objcopy -R .eeprom -O ihex $< $@
	cp $@ storage-firmware-`git describe --long`.hex
	ls -l $@


