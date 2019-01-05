#!/bin/bash

#Check return code of last executed subcommand and exit, if not 0
function check_err(){
	if [[ $? -ne 0 ]]; then
		echo "Failed to run '$1'. Please check error messages. Exiting."
		exit 1
	fi
}

#Make sure file exist
function assert_exist(){
	if [[ ! -f $1  ]]; then
		echo "File '$1' does not exist. Exiting."
		exit 1
	fi
}

#Make sure the command exist
function assert_command(){
	command -v $1 >/dev/null 2>&1 || { echo >&2 "Command '$1' required but not found. Exiting."; exit 1; }
}

SHASUM=sha256sum
if [[ $(uname) == "Darwin" ]]; then
	SHASUM=gsha256sum
fi

#Check for required commands
assert_command srec_cat
assert_command $SHASUM

#Check for command line arguments
if [ "$#" -ne 2  ]; then
        echo "usage: ./compare.sh original_firmware.hex exported_firmware.bin"
        exit 1
fi

echo "Running $0 $*"

#Check if arguments exist
assert_exist $1
assert_exist $2

#Store intermediate output to temporary folder
tmpdir=$(mktemp -d)

#Generate comparable firmware binaries with zeroes filled in
srec_cat "$1" -Intel -offset=0x80000000 -fill 0xFF 0x00000 0x3E000 -output "$tmpdir"/firmware_orig_filled.bin -binary
check_err "srec_cat with $1"
srec_cat "$2" -binary -offset=0x00000000 -fill 0xFF 0x00000 0x3E000 -output "$tmpdir"/firmware_exported_filled.bin -binary
check_err "srec_cat with $2"
assert_exist "$tmpdir"/firmware_orig_filled.bin
assert_exist "$tmpdir"/firmware_exported_filled.bin

#Generate checksums
hash_orig=$($SHASUM "$tmpdir"/firmware_orig_filled.bin | cut -d " " -f 1)
hash_expo=$($SHASUM "$tmpdir"/firmware_exported_filled.bin | cut -d " " -f 1)

if [ -e $hash_expo ] || [ -e $hash_orig  ]; then
	echo "'$SHASUM' subcommand output empty. Exiting."
	exit 1
fi

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

