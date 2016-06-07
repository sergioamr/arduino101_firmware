/* board.h - board configuration macros for the ia32 platform */

/*
 * Copyright (c) 2010-2015, Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
This header file is used to specify and describe board-level aspects for
the 'ia32' platform.
 */

#ifndef __INCboardh
#define __INCboardh

#include <misc/util.h>

#ifndef _ASMLANGUAGE
#include <drivers/rand32.h>
#endif

#ifdef CONFIG_IOAPIC
#include <drivers/ioapic.h>
#ifdef CONFIG_SERIAL_INTERRUPT_LEVEL
#ifdef CONFIG_SERIAL_INTERRUPT_LOW
#define UART_IOAPIC_FLAGS (IOAPIC_LEVEL | IOAPIC_LOW)
#else
#define UART_IOAPIC_FLAGS (IOAPIC_LEVEL)
#endif
#else /* edge triggered interrupt */
#ifdef CONFIG_SERIAL_INTERRUPT_LOW
/* generate interrupt on falling edge */
#define UART_IOAPIC_FLAGS (IOAPIC_LOW)
#else
/* generate interrupt on raising edge */
#define UART_IOAPIC_FLAGS (0)
#endif
#endif
#endif

/* programmable interrupt controller info (pair of cascaded 8259A devices) */

#define PIC_MASTER_BASE_ADRS 0x20
#define PIC_SLAVE_BASE_ADRS 0xa0
#define PIC_MASTER_STRAY_INT_LVL 0x07 /* master PIC stray IRQ */
#define PIC_SLAVE_STRAY_INT_LVL 0x0f  /* slave PIC stray IRQ */
#define PIC_MAX_INT_LVL 0x0f	  /* max interrupt level in PIC */
#define PIC_REG_ADDR_INTERVAL 1
#define INT_VEC_IRQ0 0x20 /* vector number for PIC IRQ0 */
#define N_PIC_IRQS 16     /* number of PIC IRQs */

/*
 * IO APIC (IOAPIC) device information (Intel ioapic)
 */
#define IOAPIC_NUM_RTES 24 /* Number of IRQs = 24 */

#define IOAPIC_BASE_ADRS_PHYS 0xFEC00000 /* base physical address */
#define IOAPIC_SIZE KB(4)

#define IOAPIC_BASE_ADRS IOAPIC_BASE_ADRS_PHYS

/*
 * Local APIC (LOAPIC) device information (Intel loapic)
 */
#define LOAPIC_BASE_ADRS_PHYS 0xFEE00000 /* base physical address */
#define LOAPIC_SIZE KB(4)

#define LOAPIC_BASE_ADRS LOAPIC_BASE_ADRS_PHYS

/* local APIC timer definitions */
#define LOAPIC_TIMER_IRQ IOAPIC_NUM_RTES
#define LOAPIC_TIMER_INT_PRI 2
#define LOAPIC_VEC_BASE(x) (x + 32 + IOAPIC_NUM_RTES)
#define LOAPIC_TIMER_VEC LOAPIC_VEC_BASE(0)

/* serial port (aka COM port) information */

#define COM1_BASE_ADRS 0x3f8
#define COM1_INT_LVL 0x04 /* COM1 connected to IRQ4 */
#define COM1_INT_VEC (INT_VEC_IRQ0 + COM1_INT_LVL)
#define COM1_INT_PRI 3 /* not honoured with 8259 PIC */
#define COM1_BAUD_RATE 115200

#define COM2_BASE_ADRS 0x2f8
#define COM2_INT_LVL 0x03 /* COM2 connected to IRQ3 */
#define COM2_INT_VEC (INT_VEC_IRQ0 + COM2_INT_LVL)
#define COM2_INT_PRI 3 /* not honoured with 8259 PIC */
#define COM2_BAUD_RATE 115200

#define UART_REG_ADDR_INTERVAL 1 /* address diff of adjacent regs. */
#define UART_XTAL_FREQ 1843200

/* uart configuration settings */

/* Generic definitions */
#define CONFIG_UART_NUM_SYSTEM_PORTS 2
#define CONFIG_UART_NUM_EXTRA_PORTS 0
#define CONFIG_UART_BAUDRATE COM1_BAUD_RATE
#define CONFIG_UART_NUM_PORTS \
	(CONFIG_UART_NUM_SYSTEM_PORTS + CONFIG_UART_NUM_EXTRA_PORTS)
#define CONFIG_UART_PORT_0_REGS COM1_BASE_ADRS
#define CONFIG_UART_PORT_0_IRQ COM1_INT_LVL
#define CONFIG_UART_PORT_1_REGS COM2_BASE_ADRS
#define CONFIG_UART_PORT_1_IRQ COM2_INT_LVL

#define UART_PORTS_CONFIGURE(__type, __name)			\
	static __type __name[CONFIG_UART_NUM_PORTS] = {		\
		{						\
			.port = CONFIG_UART_PORT_0_REGS,	\
			.irq = CONFIG_UART_PORT_0_IRQ		\
		},						\
		{						\
			.port = CONFIG_UART_PORT_1_REGS,	\
			.irq = CONFIG_UART_PORT_1_IRQ		\
		}						\
	}

/* Console definitions */
#define CONFIG_UART_CONSOLE_IRQ COM1_INT_LVL
#define CONFIG_UART_CONSOLE_INT_PRI COM1_INT_PRI

/* Bluetooth UART definitions */
#define CONFIG_BLUETOOTH_UART_INDEX 1
#define CONFIG_BLUETOOTH_UART_IRQ COM2_INT_LVL
#define CONFIG_BLUETOOTH_UART_INT_PRI COM2_INT_PRI
#define CONFIG_BLUETOOTH_UART_FREQ UART_XTAL_FREQ
#define CONFIG_BLUETOOTH_UART_BAUDRATE CONFIG_UART_BAUDRATE

/*
 * Programmable interval timer (PIT) device information (Intel i8253)
 *
 * The PIT_INT_VEC macro is also used when using the Wind River
 * Hypervisor "timerTick" service, whereas the PIT_INT_LVL is not.
 */
#define PIT_INT_VEC INT_VEC_IRQ0 /* PIT interrupt vector */
#define PIT_INT_LVL 0x00	 /* PIT connected to IRQ0 */
#define PIT_INT_PRI 2		 /* not honoured with 8259 PIC */
#define PIT_BASE_ADRS 0x40
#define PIT_REG_ADDR_INTERVAL 1

#ifndef _ASMLANGUAGE
/*
 * Device drivers utilize the macros PLB_BYTE_REG_WRITE() and
 * PLB_BYTE_REG_READ() to access byte-wide registers on the processor
 * local bus (PLB), as opposed to a PCI bus, for example.  Boards are
 * expected to provide implementations of these macros.
 */

#define PLB_BYTE_REG_WRITE(data, address) sys_out8(data, (unsigned int)address)
#define PLB_BYTE_REG_READ(address) sys_in8((unsigned int)address)

#define outByte(data, address) sys_out8(data, (unsigned int)address)
#define inByte(address) sys_in8((unsigned int)address)

/*
 * Device drivers utilize the macros PLB_WORD_REG_WRITE() and
 * PLB_WORD_REG_READ() to access shortword-wide registers on the processor
 * local bus (PLB), as opposed to a PCI bus, for example.  Boards are
 * expected to provide implementations of these macros.
 */

#define PLB_WORD_REG_WRITE(data, address) sys_out16(data, (unsigned int)address)
#define PLB_WORD_REG_READ(address) sys_in16((unsigned int)address)


/*
 * Device drivers utilize the macros PLB_LONG_REG_WRITE() and
 * PLB_LONG_REG_READ() to access longword-wide registers on the processor
 * local bus (PLB), as opposed to a PCI bus, for example.  Boards are
 * expected to provide implementations of these macros.
 */

#define PLB_LONG_REG_WRITE(data, address) sys_out32(data, (unsigned int)address)
#define PLB_LONG_REG_READ(address) sys_in32((unsigned int)address)

extern void _SysIntVecProgram(unsigned int vector, unsigned int);
#else /* _ASMLANGUAGE */

/*
 * Assembler macros for PLB_BYTE/WORD/LONG_WRITE/READ
 *
 * Note that these macros trash the contents of EAX and EDX.
 * The read macros return the contents in the EAX register.
 */
#define PLB_BYTE_REG_WRITE(data, address) \
	movb $data, % al;                 \
	movl $address, % edx;             \
	outb % al, % dx

#define PLB_BYTE_REG_READ(address) \
	movl $address, % edx;      \
	inb % dx, % al

#define PLB_WORD_REG_WRITE(data, address) \
	movw $data, % ax;                 \
	movl $address, % edx;             \
	outw % ax, % dx

#define PLB_WORD_REG_READ(address) \
	movl $address, % edx;      \
	inw % dx, % ax

#define PLB_LONG_REG_WRITE(data, address) \
	movl $data, % ax;                 \
	movl $address, % edx;             \
	outl % eax, % dx

#define PLB_LONG_REG_READ(address) \
	movl $address, % edx;      \
	inl % dx, % eax

#endif /* !_ASMLANGUAGE */

#endif /* __INCboardh */
