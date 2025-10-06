#!/usr/bin/python3

import sys
from time import sleep
from serial import Serial
from xmodem import XMODEM

try:
    port = Serial(sys.argv[1], 921600, timeout=5)
except IndexError:
    print(f'Usage: {sys.argv[0]} <port>')
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

print('Interrupting boot...')
if not interrupt_boot()[0]:
    port.close()
    print('Failed to interrupt boot, please reset and try again.')
    sys.exit(2)

WAIT_TIME = 0.005  # Console is slow, requires wait to process command before receiving next

print('Boot interrupt successful, obtaining console...')
port.write(b'\r')
sleep(WAIT_TIME)
if not port.read_until(b'SONiX UART Console:\r\n\0'):
    port.close()
    print('Failed to obtain console, please reset and try again.')
    sys.exit(2)

print('Running commands...')
port.write(b'w 20000180 bf000000\r')  # Set up patch for skipping hide register write
sleep(WAIT_TIME)
port.write(b'w 20000184 e0040800\r')  # Set up patch for not turning off WDT
sleep(WAIT_TIME)
port.write(b'w e0002004 180\r')       # Set remap table address
sleep(WAIT_TIME)
port.write(b'w e0002008 8005419\r')   # Set comparator 0 for 0x08005418
sleep(WAIT_TIME)
port.write(b'w e000200c 8000ef1\r')   # Set comparator 1 for 0x08000ef0
sleep(WAIT_TIME)
port.write(b'w e0002000 263\r')       # Enable FPB
sleep(WAIT_TIME)
port.write(b'w 40008000 5afa0001\r')  # Enable WDT0
sleep(WAIT_TIME)

# Console is still alive and echoing for a bit before WDT reset, so spam
# interrupt, then stop if it doesn't immediately succeed, because console will
# be unresponsive while resetting
while True:
    int_success, int_first_time = interrupt_boot()
    if not int_success:
        port.close()
        print('Failed to interrupt boot after WDT reset, please reset and try again.')
        sys.exit(2)
    
    if not int_first_time:
        break

print('Boot interrupted after WDT, obtaining console...')
port.write(b'\r')
sleep(WAIT_TIME)
if not port.read_until(b'SONiX UART Console:\r\n\0'):
    port.close()
    print('Failed to obtain console, please reset and try again.')
    sys.exit(2)

def getc(size, timeout=1):
    return port.read(size) or None

def putc(data, timeout=1):
    return port.write(data)  # note that this ignores the timeout

xm = XMODEM(getc, putc)

with open('snc7330_core0_rom.bin', 'wb') as fd:
    port.write(b'xr 08000000 10000\r')
    sleep(WAIT_TIME)
    port.read_until(b'XModem waiting for transmission...\r\n\0')
    print('Exploit OK, receiving ROM...')
    xm.recv(fd, crc_mode=0)
    print('Done')

port.close()
