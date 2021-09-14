# Nitrokey Storage Firmware Checksum Tool

## Description
Due to some peculiarities of how flash storage treats empty memory cells, a simple diff of an objdump of the downloaded firmware will look differently than the exported binary from the Nitrokey Storage. There are some extra steps needed to fill in the blank spots of the binary and make both files comparable. To make this a little more convenient, there is script that takes care of that and generates the necessary SHA256 checksums for both files.

## Usage
The script exists as a bash script for Linux and as a Batch Script for Windows. The tool relies on the use of [SRecord] to create comparable binaries.

### Windows
1. Make a new folder
2. Download the [Windows Version] and extract `srec_cat.exe` to the folder.
3. Put the downloaded firmware Hex file and the exported firmware file from the stick in that folder as well
4. `Shift+Right-Click` the folder and select `Open Command Window Here`
5. In the command window, type in `nkcompare.bat <filename of downloaded firmware>.hex firmware.bin` (with the actual name of the downloaded file filled in instead of the `<...>`)
6. You should now see the result of the comparison and the resulting hashsum(s).

If you have installed any Linux-like environment like [Git for Windows], you can also differ in Step 4. and select `Open Git Bash here` from the Context menu and follow the Linux instructions starting from Step 5.

### Linux
1. Install [SRecord] through your package manager or build it from the sources.
2. Make a new folder and put the `nkcompare.sh` script in it.
3. Put the downloaded firmware Hex file and the exported firmware file from the stick in that folder as well
4. Open a terminal and navigate to the folder
5. Make the script executable with `sudo chmod +x nkcompare.sh`
6. Invoke the script by typing `./nkcompare.sh <filename of downloaded firmware>.hex firmware.bin` (with the actual name of the downloaded file filled in instead of the `<...>`)
7. You should now see the result of the comparison and the resulting hashsum(s).

### macOS
Guide written and tested on: macOS 10.13.6.

1. Install SRecord using [homebrew]:
```
brew install srecord
```
2. If not done already, make sure the unencrypted volume is in read-write mode, export the firmware, and remove and insert back the device.
2. Make a new folder and put the `nkcompare.sh` script in it.
3. Put the downloaded firmware Hex file and the exported firmware file from the stick in that folder as well
4. Open a terminal and navigate to the folder
5. Make the script executable with `sudo chmod +x nkcompare.sh`
6. Invoke the script by typing `./nkcompare.sh <filename of downloaded firmware>.hex firmware.bin` (with the actual name of the downloaded file filled in instead of the `<...>`)
7. You should now see the result of the comparison and the resulting hashsum(s).

### Output examples for macOS / Linux
```
# download.hex - firmware file downloaded from device's site
# firmware.bin - firmware file exported from the device
# Assuming ./nkcompare.sh is in current directory

# Get firmware files (example)
# Copy exported firmware file from the Unencrypted Volume (macOS only)
cp /Volumes/Nitrokey/firmware.bin .
# Download v0.53 firmware from the firmware releases
curl -L -o download.hex https://github.com/Nitrokey/nitrokey-storage-firmware/releases/download/V0.53/storage-firmware-V0.53-0-g905976e.hex

# Checking against damaged download.hex firmware file
> ./nkcompare.sh download.hex firmware.bin
Running ./nkcompare.sh download.hex firmware.bin
srec_cat: download.hex: 2: checksum mismatch (01 != 00)
Failed to run 'srec_cat with download.hex'. Please check error messages. Exiting.

# Checking against matching firmwares
> ./nkcompare.sh download.hex firmware.bin
Running ./nkcompare.sh download.hex firmware.bin
Firmware binaries match
SHA256: aea280a50225b270b980fe81ad873d9418b449916ee1c15a01e280f484ecc80c

# Checking not matching firmwares
> ./nkcompare.sh download.hex firmware.bin
Running ./nkcompare.sh download.hex firmware.bin
Firmware binaries mismatch
Original Firmware SHA256: aea280a50225b270b980fe81ad873d9418b449916ee1c15a01e280f484ecc80c
Exported Firmware SHA256: 93befae03706222ba09013f5d85d438e57f913727afa2a7802510ab5f07c0294

```

[SRecord]: http://srecord.sourceforge.net/
[Windows Version]: https://sourceforge.net/projects/srecord/files/srecord-win32/
[Git for Windows]: https://git-scm.com/download/win
[homebrew]: https://brew.sh/
