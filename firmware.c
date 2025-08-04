// FPB registers
#define FP_CTRL        *((volatile unsigned int *)0xe0002000)
#define FP_REMAP       *((volatile unsigned int *)0xe0002004)
#define FP_COMPn       ((volatile unsigned int *)0xe0002008)

// WDT0 registers
#define WDT0_CFG       *((volatile unsigned int *)0x40008000)
#define WDT0_CLKSOURCE *((volatile unsigned int *)0x40008004)
#define WDT0_TC        *((volatile unsigned int *)0x40008008)
#define WDT0_FEED      *((volatile unsigned int *)0x4000800c)

#define WDKEY 0x5afa0000

// The magic register
#define OSC_CTRL       *((volatile unsigned int *)0x45000000)

#define REMAP_TABLE    ((unsigned int *)0x20000180)

int main(void)
{
    // Disable WDT
    WDT0_CFG = WDKEY; // WDTEN=0

    // Check if hidden ROM is open
    if ((OSC_CTRL & 0x8) != 0) {
        // Not open, set up FPB
        // Set up remap table
        REMAP_TABLE[0] = 0xbf00bf00;

        // Configure FPB
        FP_REMAP = (unsigned int)REMAP_TABLE & 0x1FFFFFE0;
        FP_COMPn[0] = 0x080054A4 | 1; // REPLACE=0, ENABLE=1
        FP_CTRL |= 3; // KEY=1, ENABLE=1

        // Start up WDT
        // WDT0_TC = WDKEY; // TC=0
        WDT0_FEED = WDKEY | 0x55aa;
        WDT0_CFG = WDKEY | 1; // WDTIE=0, WDTEN=1
    }

    // If the hidden ROM is open, we stop here without timing out the WDT.
    // Otherwise, the WDT will kick in and reset from ROM.
    while (1);

    return 0;
}
