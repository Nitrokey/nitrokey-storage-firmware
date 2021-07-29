#!/usr/bin/env python3

import click
import intelhex

FLASH_START = 0x80000000

# these paths are for the future use for parsing the user page start
PATH_LINKER = 'src/SOFTWARE_FRAMEWORK/UTILS/LINKER_SCRIPTS/AT32UC3A3/256/GCC/link_uc3a3256.lds'
PATH_HEADER = 'src/global.h'
# FIRMWARE_FILE = '../src/firmware.hex'
PAGE_SIZE = 512
USER_PAGE_START = 495 * PAGE_SIZE + FLASH_START


def round_a_to_next_b(a, b):
    a = a + b
    a = a - a % b
    return a


def get_in_kB(a):
    return (a - FLASH_START) / 1024


def calculate_occupation(firmware):
    b = (firmware.maxaddr() - FLASH_START) / 1024
    a = b / ((USER_PAGE_START - FLASH_START) / 1024) * 100
    print(f'*** Flash occupied (application region): {round(a, 1)}% ({round(b, 2)}kB)')
    if firmware.maxaddr() > USER_PAGE_START:
        print(f'!!! Overflow by {firmware.maxaddr() - USER_PAGE_START} bytes')


def bytes_from_file(filename):
    with filename as f:
        return f.read()


@click.group()
def cli():
    pass


@click.command()
# @click.argument("firmware_file", default="../src/firmware.hex", type=click.File('r'))
@click.option("--firmware-file", prompt="Firmware file path", help="Firmware file path", default="../src/firmware.hex",
              type=click.File('r'))
def status(firmware_file):
    """Print current firmware image occupation."""
    i = intelhex.IntelHex()
    i.loadfile(firmware_file, format='hex')
    b = len(i) / 1024
    a = b / (495 * 512 / 1024) * 100
    print(f'*** Flash occupied: {round(a) + 1}% ({round(b) + 1}kB)')


@click.command()
@click.option("--firmware-file", prompt="Firmware file path", help="Firmware file path", default="../src/firmware.hex",
              type=click.File('r'))
@click.option("--random-data", prompt="Random data path", help="Random data path", default="random.bin",
              type=click.File('rb'))
@click.option("--output-file-prefix", prompt="Output file name prefix", help="Output file name prefix", default="firmware-extended")
def fill_empty(firmware_file, random_data, output_file_prefix):
    """Fill empty firmware space with the provided random file data content."""
    firmware = intelhex.IntelHex()
    firmware.loadfile(firmware_file, format='hex')

    calculate_occupation(firmware)

    maximum_byte = firmware.maxaddr() + 1
    print(f'Last byte in the image: {maximum_byte - FLASH_START} ({round((maximum_byte - FLASH_START) / 1024, 2)} kB)')

    # maximum_byte += PAGE_SIZE  # leave one page extra from the firmware
    maximum_byte = round_a_to_next_b(maximum_byte, PAGE_SIZE)
    print(f"Rounded to the next page start: {(maximum_byte - FLASH_START) / 1024} kB")
    print(
        f'Writing in range {get_in_kB(maximum_byte)} kB - {get_in_kB(USER_PAGE_START)} kB ({hex(maximum_byte)} - {hex(USER_PAGE_START)})')

    # for testing
    # for i in range(maximum_byte, USER_PAGE_START):
    #     firmware[i] = 0x42
    # data = itertools.repeat(0x42, USER_PAGE_START-maximum_byte)

    data = bytes_from_file(random_data)
    data = list(data)
    space_to_fill = USER_PAGE_START - maximum_byte
    print(f'Space to fill: {space_to_fill / 1024} kB; read data: {len(data) / 1024} kB')
    firmware[maximum_byte:USER_PAGE_START] = data[0:space_to_fill]

    print('*** Testing after modification')
    calculate_occupation(firmware)

    ext = "{.bin,.hex}"
    print(f'*** Saving to {output_file_prefix}{ext}')
    with open('%s.hex' % output_file_prefix, 'w+') as f:
        firmware.write_hex_file(f)
    with open('%s.bin' % output_file_prefix, 'bw+') as f:
        firmware.tobinfile(f)


cli.add_command(fill_empty)
cli.add_command(status)

if __name__ == '__main__':
    print(f'Assuming user page starts at: {hex(USER_PAGE_START)} ({(USER_PAGE_START - FLASH_START) / 1024} kB)')
    cli()
