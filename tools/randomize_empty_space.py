import itertools

import intelhex

FLASH_START = 0x80000000

file = '../src/firmware.hex'

PAGE_SIZE = 512
USER_PAGE_START = 495 * PAGE_SIZE + FLASH_START


def round_a_to_next_b(a, b):
    a = a + b
    a = a - a % b
    return a


def get_in_kB(a):
    return (a - FLASH_START) / 1024


def calc(firmware):
    b = (firmware.maxaddr() - FLASH_START) / 1024
    a = b / ((USER_PAGE_START - FLASH_START) / 1024) * 100
    print(f'*** Flash occupied: {round(a, 1)}% ({round(b, 2)}kB)')
    if firmware.maxaddr() > USER_PAGE_START:
        print(f'!!! Overflow by {firmware.maxaddr() - USER_PAGE_START} bytes')


def bytes_from_file(filename, chunksize=8192):
    with open(filename, "rb") as f:
        while True:
            chunk = f.read(chunksize)
            if chunk:
                for b in chunk:
                    yield b
            else:
                break

def main():
    firmware = intelhex.IntelHex()
    firmware.loadfile(open(file), format='hex')

    calc(firmware)

    maximum_byte = firmware.maxaddr() + 1
    print(f'{maximum_byte} {maximum_byte / 1024}')

    # maximum_byte += PAGE_SIZE  # leave one page extra from the firmware
    maximum_byte = round_a_to_next_b(maximum_byte, PAGE_SIZE)
    print((maximum_byte - FLASH_START) / 1024)
    print(f'Writing in range {get_in_kB(maximum_byte)} - {get_in_kB(USER_PAGE_START)}')
    print(f'Writing in range {hex(maximum_byte)} - {hex(USER_PAGE_START)}')

    # for i in range(maximum_byte, USER_PAGE_START):
    #     firmware[i] = 0x42

    data = bytes_from_file('random.bin')
    # data = itertools.repeat(0x42, USER_PAGE_START-maximum_byte)
    data = list(data)
    firmware[maximum_byte:USER_PAGE_START] = data[0:USER_PAGE_START-maximum_byte]

    calc(firmware)

    firmware.write_hex_file(open('firmware-extended.hex', 'w+'))
    firmware.tobinfile(open('firmware-extended.bin', 'bw+'))


if __name__ == '__main__':
    main()
