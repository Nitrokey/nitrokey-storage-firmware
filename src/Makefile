include build.mk

GIT=$(shell git describe)
FIRMWARE=nitrokey-storage-$(GIT).hex
FIRMWARE_CLEAN=nitrokey-storage-$(GIT)-reproducible.hex
HASHFN=sha256sum

all: $(FIRMWARE_CLEAN) $(FIRMWARE) firmware_info firmware.lss $(HASHFN)
	-rm firmware.hex firmware-extended.hex
	ls -lh *hex *sha256sum $(HASHFN)
	@cat $(HASHFN)

#%.sha256sum: %
#	sha256sum $< > $@

$(HASHFN): $(FIRMWARE_CLEAN) $(FIRMWARE)
	sha256sum $^ > $@

$(FIRMWARE): firmware-extended.hex
	cp $< $@

.PHONY: clean_artifacts
clean_artifacts:
	-rm *sha256sum *hex $(HASHFN)

.PHONY: clean
clean:
	-rm $(OBJS) $(OBJSASM) $(STATEFILES)  firmware.lss

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

.PHONY: firmware_info
firmware_info: | pipenv $(FIRMWARE_CLEAN) $(FIRMWARE)
	-cd ../tools && pipenv run ./randomize_empty_space.py status --firmware-file ../src/$(FIRMWARE_CLEAN)
	-cd ../tools && pipenv run ./randomize_empty_space.py status --firmware-file ../src/$(FIRMWARE)

firmware.lss: USB_MASS.elf
	$(OBJDUMP) -h -S $< > $@

STATEFILES += pipenv
pipenv:
	-cd ../tools && pipenv --rm
	cd ../tools && pipenv install --python `which python3`
	touch $@

firmware-extended.hex: | pipenv $(FIRMWARE_CLEAN)
	cd ../tools && pipenv run ./randomize_empty_space.py fill-empty --random-data random.bin --firmware-file ../src/$(FIRMWARE_CLEAN) --output-file-name ../src/firmware-extended

$(FIRMWARE_CLEAN): USB_MASS.elf clean_artifacts
	ls -l $<
	$(OBJCOPY) -R .eeprom -O ihex $< $@

BOOTLOADER_VID := 03eb
.PHONY: flash
flash: $(FIRMWARE)
	lsusb | grep ${BOOTLOADER_VID}: || sleep 1
	lsusb | grep ${BOOTLOADER_VID}: || sleep 1
	lsusb | grep ${BOOTLOADER_VID}: || sleep 1
	sudo dfu-programmer at32uc3a3256s erase
	sleep 1
	sudo dfu-programmer at32uc3a3256s flash --suppress-bootloader-mem $<
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
	cp $(LIBNK)/unittest/report.html .

.PHONY: run_tests_quick
TESTS=status
run_tests_quick:
	cd $(LIBNK)/unittest && pipenv run pytest $(TP) test_pro.py test_storage.py -k "$(TESTS)"


.PHONY: help
help:
	@echo "Targets:"
	@grep "^[\.\w]*:" -P Makefile --color=yes

.PHONY: devloop
devloop: | all enable_bootloader flash run_tests_quick
	@echo done

RELEASEDIR=release
.PHONY: release
release: all
	mkdir -p $(RELEASEDIR)
	rm ./$(RELEASEDIR)/*
	mv -v *.hex $(HASHFN) $(RELEASEDIR)
	ls $(RELEASEDIR)

-include local.mk
