include build.mk

GIT=$(shell git describe)
FIRMWARE=nitrokey-storage-$(GIT).hex

all: firmware.hex $(FIRMWARE)
	ls -lh $^

$(FIRMWARE): firmware.hex
	cp $< $@

.PHONY: clean
clean:
	-rm $(OBJS) $(OBJSASM)

.PHONY: build-docker-toolchain
build-docker-toolchain:
	docker build .. -t nk-storage-build --build-arg TOOLCHAIN_LINK=${download_toolchain}

.PHONY: docker-make
docker-make:
	docker run -i --rm -v $(PWD)/..:/app nk-storage-build make -C src CC=/avr32-gcc

USB_MASS.elf: $(OBJS) $(OBJSASM)
	$(RUNLINK) -o$@ $^  $(RUNLINK2)
	ls -lh $@

%.o: %.c
	@echo "*** $< > $@"
	@$(RUN) -o $@ $<

%.o: %.x
	@echo "*** $< > $@"
	@$(RUNASM) -o $@ $<

firmware.hex: USB_MASS.elf
	ls -l $<
	avr-objcopy -R .eeprom -O ihex $< $@

BOOTLOADER_VID := 03eb
.PHONY: flash
flash: firmware.hex
	lsusb | grep ${BOOTLOADER_VID}: || sleep 1
	lsusb | grep ${BOOTLOADER_VID}: || sleep 1
	lsusb | grep ${BOOTLOADER_VID}: || sleep 1
	sudo dfu-programmer at32uc3a3256s erase
	sleep 1
	sudo dfu-programmer at32uc3a3256s flash --suppress-bootloader-mem firmware.hex
	sleep 1
	-sudo dfu-programmer at32uc3a3256s launch
	sleep 1
	lsusb | grep 20a0:
	date

LIBNK=
TP=-xv
.PHONY: enable_bootloader
enable_bootloader:
	cd $(LIBNK)/unittest && pytest $(TP) test_storage.py -k "enable_firmware" --run-skipped

.PHONY: run_tests
run_tests:
	cd $(LIBNK)/unittest && pipenv run pytest $(TP) test_storage.py --template=html1/index.html --report=report.html && notify-send --urgency=low "finished" || notify-send --urgency=low "failed" && xdg-open report.html && false

.PHONY: run_tests_quick
TESTS=encrypted
run_tests_quick:
	cd $(LIBNK)/unittest && pipenv run pytest $(TP) test_storage.py -k "$(TESTS)"


.PHONY: help
help:
	@echo "Targets:"
	@grep "^[\.\w]*:" -P Makefile --color=yes

.PHONY: devloop
devloop: | all enable_bootloader flash
	@echo done


-include local.mk
