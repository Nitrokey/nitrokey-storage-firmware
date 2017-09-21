Nitrokey Storage Firmware
=========================

## Building

### Windows
Note: Tested with Windows 7

Install the following tools in this order:

1. [avr32-gnu-toolchain-2.4.2-setup.exe](http://www.atmel.com/System/BaseForm.aspx?target=tcm:26-17439)
2. [avr32studio-ide-installer-2.5.0.35-win32.win32.x86.exe](http://www.atmel.com/tools/studioarchive.aspx)
3. [AvrStudio4Setup.exe](http://www.atmel.com/tools/studioarchive.aspx)
4. [AVRStudio4.18SP2.exe](http://www.atmel.com/System/BaseForm.aspx?target=tcm:26-41051)

### Linux

The compile procedure is as follows (tested on ArchLinux but should work on any
other GNU/Linux OS):
1. Clone this git project (`git clone https://github.com/Nitrokey/nitrokey-storage-firmware.git`)
2. Download and extract [AVR32 Studio](http://www.atmel.com/tools/Archive/AVR32STUDIO2_6.aspx). Example archive filename: `avr32studio-ide-2.6.0.753-linux.gtk.x86_64.zip`. At the moment no newer version seems to be available for Linux.
3. Start AVR32 Studio by executing `avr32studio` in the extracted folder.
4. Import project into AVR32 Studio: File | Import... | General | Existing Projects into Workspace | Choose the folder of downloaded git project.
5. Rename `pm_240.h` in the git project folder to `pm_231.h`. Make a backup of `as4e-ide/plugins/com.atmel.avr.toolchains.linux.x86_64_3.0.0.201009140852/os/linux/x86_64/avr32/include/avr32/pm_231.h`. Replace that `pm_231.h` with the renamed `pm_240.h`.
6. In AVR32 Studio select Project | Build All.
7. The builded file is now in the folder `Debug` of the git project folder.

#### Converting to .HEX file
Before flashing there may be a need to convert binary file to .hex. If it was not done automatically execute the following in Debug or Release directory:
```
avr-objcopy -R .eeprom -O ihex USB_MASS.elf firmware.hex
```

## Flashing the Firmware to Device

- [Latest firmware releases](https://github.com/Nitrokey/nitrokey-storage-firmware/releases/latest)
- [Firmware upgrade instructions](https://www.nitrokey.com/en/doc/firmware-update-storage)
