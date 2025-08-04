Extracting Hidden Bootrom from Sonix SNC7330
============================================

## Article

While exploring the Tamagotchi Paradise, I dumped the bootrom of its microcontroller,
a Sonix SNC73410, but noticed something weird. When I loaded the image up in 
Ghidra, the reset handler seems to call into the middle of some code, which looks
like some sort of UART thing, and this is before what's normally the main function.
The bootrom is at 0x08000000 in memory and 64KB long. However, after running an
analysis, there seems to be calls past that, and looping the address around, the
code it landed on didn't make any sense, along with various broken references.
Upon closer inspection, it seems the data from 0x08008000 to 0x0800bfff actually
just mirrored what's at the start of the ROM, and comparing against the previous
generation SNC7320 that was found in Tamagotchi Pix, it was clear that the code
in this region was for loading user code from storage, along with any code
encryption logic as hinted to by the SDK.

After noticing this, I started looking at what could be triggering this part of
the ROM to be hidden. Since there are calls to functions within the hidden segment,
even at the start of the reset handler, it means this segment must be accessible
to the CPU at reset, and is hidden at some later point before entering the user
application. After a bit of inspection, I found that there is a set to the
`OSC_CTRL` register and another one that was documented to mux some pins to SWD
before various code that looks to hand control over to the user, whether to user
code or the built-in bootloader. It looks like this:

```
0800549e 4f f0 8a 40     mov.w      r0,#0x45000000
080054a2 01 68           ldr        r1,[r0,#0x0]
080054a4 41 f0 08 01     orr        r1,r1,#0x8
080054a8 01 60           str        r1,[r0,#0x0]
080054aa 01 6a           ldr        r1,[r0,#0x20]
080054ac 21 f0 40 01     bic        r1,r1,#0x40
080054b0 01 62           str        r1,[r0,#0x20]
```

In Sonix's SDK terms, the code is this:

```c
SN_SYS0->OSC_CTRL |= 0x8;
SN_SYS0->PINCTRL_b.SWD_SWO = 0;
```

So if I wanted to access the the hidden ROM, I need to make sure the bit in
`OSC_CTRL` is not set.

### Experimentation

First thing I tried was to unset the bit in the register directly. Unfortunately,
no such luck. It seems the bit is set-only, and cannot be cleared aside from
after a reset. I then tried a system reset via `SYSRESETREQ`, but all that did
was kick me off SWD. So it seems they also disable SWD at reset, hence setting
to mux SWD after the ROM has been hidden.

I wrote and loaded a program that enables SWD as the first thing, and tried to
`SYSRESETREQ` again. The way software reset usually works on this chip is the
start of memory would be mapped to the PRAM (program RAM), and the bootrom
would load your code in there and then jump to it, which I later found out could
be either via a `SYSRESETREQ` or directly setting the stack pointer and jumping
to the reset handler. So I tried to issue a reset within the program, and the
hidden ROM was still not visible (which should have been obvious it wouldn't
work in hindsight since the bootrom resetting into the user code and resetting
the hiding would defeat its purpose). So now, what's left?

According to the chip's datasheet, reset is handled as follows:

> A system reset is generated when one of the following events occurs.
> - Low voltage reset (LVR)
>   - 0.9V for cores during power-on
>   - 1.8V for I/O during power-on
>   - 2.1V for I/O in Normal mode
>   - Restart program from ROM after the reset signal releases
> - RST pin (external reset)
>   - Restart program from ROM after the reset signal releases
> - DPD wakeup reset
>   - When waking up from DPD mode, the system resets and restarts from ROM
> - WDT reset
>   - Reset and restart program from ROM
> - Software reset
>   - Reset and restart from PRAM

It looks like basically all reset except for software reset will restart from
ROM, which necessitates the ROM being unhidden. In this case, it seems the
watchdog reset would be the easiest reset to trigger.

### What if we just don't hide?

At this point, it seems the only reasonable choice is to stop the hiding from
occurring in the first place, since SWD is not available until after the hiding,
and if you reset you just lose SWD again, plus all user code is only executed
after the hiding has been set.

Luckily for me, the Cortex-M3 has just the mechanism for this use case. The
Flash Patch and Breakpoint (FPB) unit is a fairly standard peripheral, and upon
examining it, I confirmed that it does have patching capabilities. The FPB only
gets reset on a power-on reset, so theoretically if I can trigger any other kind
of reset, it would retain its configuration, and let me potentially patch the
ROM code. The FPB supports breaking/patching memory addresses from 0x00000000
to 0x1fffffff on a 4-byte boundary, and the remap table can be anywhere within
0x20000000 to 0x3fffffff. This covers the ROM range. Also a lucky coincidence,
the instruction for setting the bit for hiding the ROM is on a 4-byte boundary,
so I can just replace it with two `NOP`s to negate its effect.

### Setting up the trap

To set up the FPB:

1. Set up the remap table. This contains the data the CPU should read instead of
   the original. It's an array with 32-bit entries, each of which corresponds to
   the index of the comparator. I'm just using the first slot, loading two `NOP`s,
   each consisting of the bytes `0x00 0xbf`, which becomes `0xbf00bf00` when
   written as a word in C. The location is within the mailbox RAM since that's
   the only RAM within range. It's put towards the end so it doesn't get cleared
   out by any potential mailbox queue initialization.
2. Set up the comparator. Basically you set the address that you want to watch,
   then set the top two bits (the `REPLACE` field) to `0` to indicate a remap,
   and set lowest bit `ENABLE` to 1.
3. Enable the FPB: `or` it with the value `3`, for `KEY` and `ENABLE`, and
   everything is ready.

Afterwards, enable the watchdog and wait for it to time out, and this should
trigger a WDT reset, which should unhide the ROM and restart from ROM code.
With any luck, the FPB is active and the hide register set will be skipped, so
once SWD is enabled, I can just read out the full ROM.

Because I wasn't sure if the FPB configuration would be retained properly, I set
the code up to basically reinitialize the FPB and time out until it detects that
the ROM hide bit in the `OSC_CTRL` register is no longer set.

### Testing it out

In the initial implementation, it seemed the FPB didn't retain the configuration
well and the program would just keep on looping. Trying to figure out whether it
has succeeded at all, I kept on reading the `OSC_CTRL` register in OpenOCD when
SWD was open, and noticed sometimes the bit is unset. Going on this bit of info,
I started repeatedly dumping the affected ROM segment until finally one dump
showed different data, which was the hidden contents.

To verify that it was actually the FPB and not a fluke, I rewrote the program
and tested some variations, and discovered that yes, indeed the FPB has worked,
and it was most likely my debugger interfering with it and potentially resetting
values, because if I reset the microcontroller without the debugger connected,
the next time I connect, my code would actually stop on the check that the bit
is unset, and I could access the hidden ROM. Removing the FPB setup, of course,
resulted in the ROM always being hidden and the code never stopped looping.

### Conclusion

As with other attacks based of the FPB, hardware makers often forget that it's
present, and if you don't have any countermeasures against it, someone could
possibly use it to break your security. Remember to include it in your threat
model.

Also, hiding things is security by obscurity, and it won't hold up under scrutiny.
However, sometimes there are even more egregious oversights, like the AES
peripheral in this chip not being turned off after a failed attempt to decrypt
boot code, which resulted in me being able to retrieve the configuration used
for decryption, and taken together with the boot code decryption algorithm in
the previous gen SNC7320 (that was not hidden), allowed me to reverse engineer
this chip's boot code decryption without having the full bootrom.

## Usage

Run `make`. Provide your ARM toolchain's `bin` directory path using the `BINPATH`
variable if necessary when running `make`. The resulting `firmware.bin` can be
flashed to your target.

Run the microcontroller without a debugger connected. Then connect over SWD, and
dump memory `0x08000000` for `0x10000` bytes. This should contain the full
bootrom. Confirm that the data at `0x8000` is different than at the start of the
dump. If your target continually resets, it means the code did not succeed in
preventing the register from being set, and you should disconnect your debugger
and perform an external reset or power-on reset.

## Other notes

In the load table, I've set the header to not check the boot code CRC so no
recalculation has to be done. The table's own checksum doesn't have to be valid
either, because the bootrom will accept it if after reading the table a second
time it's identical. The no `SYSRESETREQ` option was selected to reduce the
possibility of things getting unexpectedly reset on entering user application.
We don't need it anyway because we're really only operating within the processor
core.

In the startup code, pin muxing SWD is performed as the first thing. This might
not be necessary, since the bootrom should already have muxed, but it's there
just in case.

Sonix's M3B SDK is publicly available on the product pages of SNC7330/7340 series
chips, e.g https://www.sonix.com.tw/article-en-5180-42810

## Acknowledgements

Startup code and linker script stolen from https://github.com/pulkomandy/stm32f3,
with a few adjustments added for SNC7330 and including debugging sections.
