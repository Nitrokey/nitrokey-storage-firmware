#!/bin/bash

#Check for srec_cat
command -v srec_cat >/dev/null 2>&1 || { echo >&2 "srec_cat required but not found"; exit 1; }

#Check for command line arguments
if [ "$#" -ne 2  ]; then
        echo "usage: ./compare.sh original_firmware.hex exported_firmware.bin"
        exit 1
fi

#Store intermediate output to temporary folder
tmpdir=$(mktemp -d)

#Generate comparable firmware binaries with zeroes filled in
srec_cat "$1" -Intel -offset=0x80000000 -fill 0xFF 0x00000 0x3E000 -output "$tmpdir"/firmware_orig_filled.bin -binary
srec_cat "$2" -binary -offset=0x00000000 -fill 0xFF 0x00000 0x3E000 -output "$tmpdir"/firmware_exported_filled.bin -binary

#Generate checksums
hash_orig=$(sha256sum "$tmpdir"/firmware_orig_filled.bin | cut -d " " -f 1)
hash_expo=$(sha256sum "$tmpdir"/firmware_exported_filled.bin | cut -d " " -f 1)

rm -r "$tmpdir"

#tput colors for ouput
RED=$(tput setaf 1)
GREEN=$(tput setaf 2)
NORMAL=$(tput sgr0)

#Compare hash values
if [ "$hash_orig" = "$hash_expo" ]; then
    printf "${GREEN}Firmware binaries match${NORMAL}\nSHA256: %s\n" "$hash_orig"
else
    printf "${RED}Firmware binaries mismatch${NORMAL}\nOriginal Firmware SHA256: %s\nExported Firmware SHA256: %s\n" "$hash_orig" "$hash_expo"
fi
