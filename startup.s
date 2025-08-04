/* Startup for STM32F303 Cortex-M4 ARM MCU
Copyright 2012, Adrien Destugues <pulkomandy@gmail.com>
This file is distributed under the terms of the MIT licence.
 */

/* Some mandatory setup for all assembly files. We tell the compiler we use the
   unified syntax (you always want this one), the thumb instruction code (the
   only one available for Cortex-M), and we select the .text section as we will
   be writing code, not data
*/
		.syntax 	unified
		.thumb
		.text
		.section .boot,"x"

/* Export these symbols: _start is the interrupt vector, Reset_handler does the
   initialization before calling main(), and Default_Handler does nothing. It's
   used for the other interrupts besides reset. */
		.global		_start
		.global		Reset_Handler
		.global		Default_Handler

/* Import these. They are defined either in the link script or in C code
   somewhere else. */

		.extern		__text_end__
		.extern		__data_beg__
		.extern		__data_end__
		.extern		__bss_beg__
		.extern		__bss_end__
		.extern		__stack_end__
		.extern		__ctors_start__
		.extern		__ctors_end__
		.extern		main

/*===========================================================================*/

/* Use Default_handler for all exceptions and interrupts, unless another
handler is provided elsewhere.
*/

		.macro		IRQ handler
		.word		\handler	/* Store the pointer to handler here */
		.weak		\handler	/* Make it a weak symbol so it can be redefined */
		.set		\handler, Default_Handler /* And set the default value */
		.endm

/*===========================================================================*/

/* Exception vector table--Common to all Cortex-M4 */

_start: 	.word		__stack_end__	/* The stack is set up by the CPU using this at reset */
		.word		Reset_Handler		/* Then it will jump to this address */

		IRQ		NMI_Handler				// Define all other handlers to the
		IRQ		HardFault_Handler		// default value, using the IRQ macro
		IRQ		MemManage_Handler		// we defined above
		IRQ		BusFault_Handler
		IRQ		UsageFault_Handler
		.word		0
		.word		0
		.word		0
		.word		0
		IRQ		SVC_Handler
		IRQ		DebugMon_Handler
		.word		0
		IRQ		PendSV_Handler
		IRQ		SysTick_Handler

/* Hardware interrupts specific to the STM32F405
   TODO review this and see if the STM32F303 is close enough ! 
 
 NOTE: you can comment all of this if you don't use interrupts, to save a bit of
 flash space. Or if you are crazy you can even interleave code and vectors.*/

		IRQ		Core0_Issue_IRQHandler      // 16+ 0: M3_0 dual core
		IRQ		Core1_Issue_IRQHandler      // 16+ 1: M3_1 dual core
		IRQ		RTC_IRQHandler              // 16+ 2: RTC
		IRQ		WKP_IRQHandler              // 16+ 3: WKP
		.word	0                           // 16+ 4: Reserved
		.word	0                           // 16+ 5: Reserved
		IRQ		USBDEV_IRQHandler           // 16+ 6: USBDEV
		.word	0                           // 16+ 7: Reserved
		.word	0                           // 16+ 8: Reserved
		IRQ		SAR_ADC_IRQHandler          // 16+ 9: SAR_ADC
		.word	0                           // 16+10: Reserved
		.word	0                           // 16+11: Reserved
		.word	0                           // 16+12: Reserved
		.word	0                           // 16+13: Reserved
		.word	0                           // 16+14: Reserved
		IRQ		SDIO_IRQHandler             // 16+15: SDIO
		IRQ		SDIO_DMA_IRQHandler         // 16+16: SDIO_DMA
		.word	0                           // 16+17: Reserved
		.word	0                           // 16+18: Reserved
		IRQ		IDMA0_IRQHandler            // 16+19: IDMA0
		IRQ		IDMA1_IRQHandler            // 16+20: IDMA1
		IRQ		I2S0_DMA_IRQHandler         // 16+21: I2S0_DMA
		.word	0                           // 16+22: Reserved
		IRQ		I2S2_DMA_IRQHandler         // 16+23: I2S2_DMA
		.word	0                           // 16+24: Reserved
		IRQ		I2S4_IRQHandler             // 16+25: I2S4
		IRQ		GPIO0_IRQHandler            // 16+26: GPIO0
		IRQ		GPIO1_IRQHandler            // 16+27: GPIO1
		IRQ		GPIO2_IRQHandler            // 16+28: GPIO2
		.word	0                           // 16+29: Reserved
		.word	0                           // 16+30: Reserved
		IRQ		I2C0_IRQHandler             // 16+31: I2C0
		IRQ		I2C1_IRQHandler             // 16+32: I2C1
		.word	0                           // 16+33: Reserved
		IRQ		SPI0_IRQHandler             // 16+34: SPI0
		.word	0                           // 16+35: Reserved
		IRQ		UART0_IRQHandler            // 16+36: UART0
		IRQ		UART1_IRQHandler            // 16+37: UART1
		IRQ		CT32B0_IRQHandler           // 16+38: CT32B0
		IRQ		CT32B1_IRQHandler           // 16+39: CT32B1
		IRQ		CT32B2_IRQHandler           // 16+40: CT32B2
		IRQ		CT32B3_IRQHandler           // 16+41: CT32B3
		IRQ		CT32B4_IRQHandler           // 16+42: CT32B4
		IRQ		CT32B5_IRQHandler           // 16+43: CT32B5
		IRQ		CT32B6_IRQHandler           // 16+44: CT32B6
		IRQ		CT32B7_IRQHandler           // 16+45: CT32B7
		IRQ		SPI1_DMA_IRQHandler         // 16+46: SPI1_DMA
		IRQ		SPI1_ECC_IRQHandler         // 16+47: SPI1_ECC
		.word	0                           // 16+48: Reserved
		.word	0                           // 16+49: Reserved
		.word	0                           // 16+50: Reserved
		.word	0                           // 16+51: Reserved
		.word	0                           // 16+52: Reserved
		IRQ		CRC_NDT_IRQHandler          // 16+53: CRC_NDT_IRQHandler
		IRQ		USB_suspend_IRQHandler      // 16+54: USB suspend
		.word	0                           // 16+55: Reserved
		.word	0                           // 16+56: Reserved
		.word	0                           // 16+57: Reserved
		IRQ		FTDMA_IRQHandler            // 16+58: FTDMAC
		.word	0                           // 16+59: Reserved
		.word	0                           // 16+60: Reserved
		.word	0                           // 16+61: Reserved
		.word	0                           // 16+62: Reserved
		IRQ		SysClock_Change_IRQHandler  // 16+63: System Clock Change Handler(Sowftware)

/*===========================================================================*/

		.section .text,"x"
/* Default exception handler--does nothing but return */

		.thumb_func
Default_Handler: 
		b		.

/*===========================================================================*/

/* Reset vector: Set up environment to call C main() */

		.thumb_func
Reset_Handler:

/* Enable SWD */

		ldr r0, =0x45000020
		ldr r1, =0x00000020
		str r1, [r0]

/* Reload SP(R13) to correct jump info */

		ldr r0, =0xE000ED08
		ldr r0, [r0]
		ldr r13, [r0]

/* Copy initialized data from flash to RAM */

copy_data:	ldr		r1, DATA_BEG
		ldr 		r2, TEXT_END
		ldr 		r3, DATA_END
		subs		r3, r3, r1		/* Length of initialized data */
		beq		zero_bss		/* Skip if none */

copy_data_loop: ldrb		r4, [r2], #1		/* Read byte from flash */
		strb		r4, [r1], #1  		/* Store byte to RAM */
		subs		r3, r3, #1  		/* Decrement counter */
		bgt 		copy_data_loop		/* Repeat until done */

/* Zero uninitialized data (bss) */

zero_bss: 	ldr 		r1, BSS_BEG
		ldr 		r3, BSS_END
		subs 		r3, r3, r1		/* Length of uninitialized data */
		beq		zero_bss_end		/*Skip if none */

		mov 		r2, #0

zero_bss_loop: 	strb		r2, [r1], #1		/* Store zero */
		subs		r3, r3, #1		/* Decrement counter */
		bgt		zero_bss_loop		/* Repeat until done */

zero_bss_end:

@ /* Call C++ constructors.  The compiler and linker together populate the .ctors
@ code section with the addresses of the constructor functions. */

@ call_ctors:	ldr		r0, CTORS_BEG
@ 		ldr		r1, CTORS_END
@ 		subs		r1, r1, r0		/* Length of ctors section */
@ 		beq		call_main		/* Skip if no constructors */

@ ctors_loop:	ldr		r2, [r0], #4		/* Load next constructor address */
@ 		push		{r0,r1}			/* Save registers */
@ 		blx		r2			/* Call constructor */
@ 		pop		{r0,r1}			/* Restore registers */
@ 		subs		r1, r1, #4		/* Decrement counter */
@ 		bgt		ctors_loop		/* Repeat until done */

/* Skip calling System_Init */

/* Call main() */

call_main:	mov		r0, #0			/* argc=0 */
		mov		r1, #0			/* argv=NULL */

		ldr		r2, =main
		bx		r2 

/*=============================================================================*/

/* These are filled in by the linker */

		.align		4
TEXT_END:	.word		__text_end__
DATA_BEG:	.word		__data_beg__
DATA_END:	.word		__data_end__
BSS_BEG:	.word		__bss_beg__ 
BSS_END:	.word		__bss_end__
CTORS_BEG:	.word		__ctors_start__
CTORS_END:	.word		__ctors_end__

/*==========================================================================*/

/* libstdc++ needs this */

		.bss
		.align		4
__dso_handle:	.word
		.global		__dso_handle
		.weak		__dso_handle

		.end
