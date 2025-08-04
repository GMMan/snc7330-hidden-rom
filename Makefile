BINPATH	?= 

CC	= ${BINPATH}arm-none-eabi-gcc
CFLAGS	+= -Os -g -mcpu=cortex-m3 -ffreestanding

LD	= ${BINPATH}arm-none-eabi-ld
OBJCOPY	= ${BINPATH}arm-none-eabi-objcopy

OBJDIR	= obj

.PHONY: all clean

all: firmware.bin

%.bin: $(OBJDIR)/%.elf
		$(OBJCOPY) -O binary $< $@

$(OBJDIR)/firmware.elf: firmware.ld $(OBJDIR)/sonix_load_table.o $(OBJDIR)/startup.o $(OBJDIR)/firmware.o
		$(LD) -T $^ -o $@

$(OBJDIR)/%.o: %.s
		$(CC) -o $@ $(CFLAGS) -c $<

$(OBJDIR)/%.o: %.c
		$(CC) -o $@ $(CFLAGS) -c $<

clean:
		$(RM) $(OBJDIR)/* *.bin
