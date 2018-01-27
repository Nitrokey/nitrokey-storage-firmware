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

## Debugging
**Note: To connect an external debugger as described here, you will need a development version of the Nitrokey Storage that makes the JTAG pins available (pictured below). This version is currently not for sale.**

![NK Storage Development Version](/img/nkstorage_jtag.jpg "Nitrokey Storage Development Version")

### Compatible Debuggers
This has been tested with the [AVR JTAGICE XPII](https://www.waveshare.com/product/mcu-tools/avr/programmers-debuggers/usb-avr-jtagice-xpii.htm), however the more recent [Atmel ICE](http://www.microchip.com/DevelopmentTools/ProductDetails.aspx?PartNO=atatmel-ice) and any other AVR UC3 compatible debugger should work as well.

### Prepare connections
The JTAG connections on the PCB have a pitch of 1.27mm. To ease connecting and disconnecting, it is easiest to solder a pin header to the PCB and use a pin socket to quickly attach the device to the debugger. It is suggested to use the following parts for that:

| Part                                  | Digikey Part Number       |
|---                                    |---                        |
| 7-pin THT Pin header, 1.27mm Pitch    |  S9014E-07-ND             |
| 7-pin THT Pin header, 2.54mm Pitch    |  S1012EC-07-ND            |
| 7-pin Socket, 1.27mm Pitch            |  S9008E-07-ND             |
| 1.27mm Ribbon Cable, ca. 15cm         |                           |
| Heatshrink                            |                           |

- Solder the 1.27mm Pin header to the board
- Connect the 1.27mm socket and 2.54mm header to the cable and isolate individual contacts with heatshrink

### Connect Debugger interface to the Nitrokey

Use jumper wires to connect the cable from the Nitrokey to the Debugger interface connector as pictured below:

![NK Storage Debugger Connection](/img/debugger_connection.png)

| Nitrokey Side                         | AVR JTAG Connector Side   |
|---                                    |---                        |
| RST                                   | nSRST                     |
| TCK                                   | TCK                       |
| TDI                                   | TDI                       |
| TDO                                   | TDO                       |
| TMS                                   | TMS                       |
| GND                                   | GND                       |
| VDD                                   | VTref                     |

The device still needs to be powered via USB during debugging.
For an initial function test, you can issue the following commands from the AVR32Studio home directory:

```
cd /plugins/com.atmel.avr.utilities.linux.x86_64_3.0.0.201009140848/os/linux/x86_64/bin
./avr32program --part UC3A3256S cpuinfo
```
if the device is connected correctly, this should yield an output similar to this:
```
Connected to JTAGICE mkII version 6.6, 6.6 at USB.

Device information:
Device Name                                   UC3A3256S 
Device Revision                               H
JTAG ID                                       0x7202003f
SRAM size                                     128 kB
Flash size                                    256 kB
```

### Using the debugger in AVR32Studio

To enable the debugger, follow these steps inside the IDE:
- Enable the "AVR Targets" dialog under `Window -> Show View -> AVR Targets`
- Right click inside the "AVR Targets" window and select `Scan Targets`. Your debugger should now be shown as a target.
- Right click on the debugger entry and select `Properties`
- Select the "Details" tab. Under "Device", select `AVR UC3A Series -> AT32UC3A3256S`

The debugger should now be available. Configure the debugging environment by following these steps:
- Open `Run -> Debug Configurations`
- If there is no entry under `AVR Application`, create one by double clicking on it. Otherwise select the existing entry.
- Under "File", select `Debug/USB_MASS.elf`
- Under "Target" select `JTAGICE mkII` (or your correspondig JTAG debugger)
- Under "Erase Options" select `Erase sectors`
- Under "Run Options" select `Reset MCU`
- Apply Settings and close the dialog window

Congratulations, your IDE should now be ready for debugging. Set breakpoints as needed and start a JTAG debugging session by pressing F11.

### Defaulting to USB DFU
If you accidentally erased the DFU bootloader from the chip or run into any trouble, the stick can always be reproggrammed like this:
- In the "AVR Targets" dialog, right click on your JTAG debugger and select `Program Bootloader`
- Leave all the entries in their default state and reprogram the bootloader by clicking `Finish`

The device will now start in DFU mode and can be programmed as described above in the Firmware Upgrade Instructions
