/* system.c - system/hardware module for the ia32 platform */

/*
 * Copyright (c) 2011-2015, Wind River Systems, Inc.
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
This module provides routines to initialize and support board-level hardware
for the ia32 platform.
 */

#include <nanokernel.h>
#include "board.h"
#include <drivers/uart.h>
#include <drivers/pic.h>
#include <device.h>
#include <init.h>

#if defined(CONFIG_PIC) || defined(CONFIG_SHUTOFF_PIC)
#define picInit() _i8259_init()
#else
#define picInit()         \
	do {/* nothing */ \
	} while ((0))
#endif /* CONFIG_PIC */

#ifdef CONFIG_LOAPIC
#include <drivers/loapic.h>
static inline void loapicInit(void)
{
	_loapic_init();
}
#else
#define loapicInit()      \
	do {/* nothing */ \
	} while ((0))
#endif /* CONFIG_LOAPIC */

#ifdef CONFIG_IOAPIC
#include <drivers/ioapic.h>
static inline void ioapicInit(void)
{
	_ioapic_init();
}

#define uartIrqProg(irq) \
	_ioapic_irq_set((irq), (irq) + INT_VEC_IRQ0, UART_IOAPIC_FLAGS)
#else
#define ioapicInit(mask)  \
	do {/* nothing */ \
	} while ((0))
#define uartIrqProg(irq)  \
	do {/* nothing */ \
	} while ((0))
#endif /* CONFIG_IOAPIC */

#if defined(CONFIG_PRINTK) || defined(CONFIG_STDOUT_CONSOLE)

/**
 *
 * @brief Initialize initialization information for one UART
 *
 * @return N/A
 *
 */

void uart_generic_info_init(struct uart_init_info *p_info)
{
	p_info->options = 0;
	p_info->sys_clk_freq = UART_XTAL_FREQ;
	p_info->baud_rate = CONFIG_UART_BAUDRATE;
	p_info->int_pri = CONFIG_UART_CONSOLE_INT_PRI;
}


/**
 *
 * @brief Initialize target-only console
 *
 * Only used for debugging.
 *
 * @return N/A
 *
 */

#include <console/uart_console.h>

static void consoleInit(void)
{
	struct uart_init_info info;

	uart_generic_info_init(&info);
	uart_init(CONFIG_UART_CONSOLE_INDEX, &info);
	uart_console_init();
}

#else
#define consoleInit()     \
	do {/* nothing */ \
	} while ((0))
#endif /* defined(CONFIG_PRINTK) || defined(CONFIG_STDOUT_CONSOLE) */

#if defined(CONFIG_BLUETOOTH)
#if defined(CONFIG_BLUETOOTH_UART)
#include <bluetooth/uart.h>
#endif /* CONFIG_BLUETOOTH_UART */
static void bluetooth_init(void)
{
#if defined(CONFIG_BLUETOOTH_UART)
	bt_uart_init();
#endif
}
#else
#define bluetooth_init()	\
	do {/* nothing */	\
	} while ((0))
#endif /* CONFIG_BLUETOOTH */

/**
 *
 * @brief Perform basic hardware initialization
 *
 * Initialize the interrupt controller and UARTs present in the
 * platform.
 *
 * @return 0
 */
static int pc_init(struct sys_device *arg)
{
	ARG_UNUSED(arg);

	picInit();    /* NOP if not needed */
	loapicInit(); /* NOP if not needed */

	/*
	 * IOAPIC is initialized with empty interrupt list
	 * If there is a device connected to IOAPIC, the initialization
	 * has to be changed to _IoApicInit (1 << DEV1_IRQ | 1 << DEV2_IRQ)
	 */
	ioapicInit();   /* NOP if not needed */
	consoleInit(); /* NOP if not needed */
	bluetooth_init(); /* NOP if not needed */
	return 0;
}

DECLARE_DEVICE_INIT_CONFIG(pc_0, "", pc_init, NULL);
pure_early_init(pc_0, NULL);
