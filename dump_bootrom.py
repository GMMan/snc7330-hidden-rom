#!/usr/bin/python3

import sys
from time import sleep
from serial import Serial
from xmodem import XMODEM


WAIT_TIME = 0.005  # Console is slow, requires wait to process command before receiving next


try:
    port = Serial(sys.argv[1], 921600, timeout=5)
except IndexError:
    print(f'Usage: {sys.argv[0]} <port>', file=sys.stderr)
    sys.exit(1)


def interrupt_boot():
    interrupt_pattern = b'\xab\x5d\xeb\xef'
    attempts_threshold = 1000
    attempts_remaining = attempts_threshold

    orig_timeout = port.timeout
    port.timeout = 0.01
    port.reset_input_buffer()

    while attempts_remaining > 0:
        port.write(interrupt_pattern)
        if port.read(len(interrupt_pattern)) == interrupt_pattern:
            port.timeout = orig_timeout
            return (True, attempts_remaining == attempts_threshold)

        attempts_remaining -= 1

    port.timeout = orig_timeout
    return (False, False)


def send_cmd(cmd=b''):
    port.write(cmd + b'\r')
    sleep(WAIT_TIME)


print('Interrupting boot...')
if not interrupt_boot()[0]:
    port.close()
    print('Failed to interrupt boot, please reset and try again.', file=sys.stderr)
    sys.exit(2)


print('Boot interrupt successful, obtaining console...')
send_cmd()
if not port.read_until(b'SONiX UART Console:\r\n\0'):
    port.close()
    print('Failed to obtain console, please reset and try again.', file=sys.stderr)
    sys.exit(2)


print('Running commands...')
send_cmd(b'w 20000180 bf000000')  # Set up patch for skipping hide register write
send_cmd(b'w 20000184 e0040800')  # Set up patch for not turning off WDT
send_cmd(b'w e0002004 180')       # Set remap table address
send_cmd(b'w e0002008 8005419')   # Set comparator 0 for 0x08005418
send_cmd(b'w e000200c 8000ef1')   # Set comparator 1 for 0x08000ef0
send_cmd(b'w e0002000 263')       # Enable FPB
send_cmd(b'w 40008000 5afa0001')  # Enable WDT0


# Console is still alive and echoing for a bit before WDT reset, so spam
# interrupt, then stop if it doesn't immediately succeed, because console will
# be unresponsive while resetting
while True:
    int_success, int_first_time = interrupt_boot()
    if not int_success:
        port.close()
        print('Failed to interrupt boot after WDT reset, please reset and try again.', file=sys.stderr)
        sys.exit(2)

    if not int_first_time:
        break


print('Boot interrupted after WDT, obtaining console...')
send_cmd()
if not port.read_until(b'SONiX UART Console:\r\n\0'):
    port.close()
    print('Failed to obtain console, please reset and try again.', file=sys.stderr)
    sys.exit(2)


def getc(size, timeout=1):
    return port.read(size) or None

def putc(data, timeout=1):
    return port.write(data)  # note that this ignores the timeout

xm = XMODEM(getc, putc)


with open('snc7330_core0_rom.bin', 'wb') as fd:
    send_cmd(b'xr 08000000 10000')
    port.read_until(b'XModem waiting for transmission...\r\n\0')
    print('Exploit OK, receiving ROM...')
    xm.recv(fd, crc_mode=0)


print('Done')
port.close()
