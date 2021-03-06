/* system.c - system/hardware module for the Quark_SE BSP */

/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.
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
This module provides routines to initialize and support board-level
hardware for the Quark_SE BSP.
*/

#include <nanokernel.h>
#include <misc/printk.h>
#include <misc/__assert.h>
#include "board.h"
#include <drivers/uart.h>
#include <drivers/ioapic.h>
#include <drivers/loapic.h>
#include <init.h>

#if defined(CONFIG_PRINTK) || defined(CONFIG_STDOUT_CONSOLE)
#include <console/uart_console.h>

/**
 *
 * @brief Initialize initialization information for one UART
 *
 * RETURNS: N/A
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
 * consoleInit - initialize target-only console
 *
 * Only used for debugging, no host driver involved.
 *
 * RETURNS: N/A
 *
 */
static void consoleInit(void)
{

        struct uart_init_info info;

        uart_generic_info_init(&info);
        uart_init(CONFIG_UART_CONSOLE_INDEX, &info);
        uart_console_init();

}

#else
#define consoleInit() do { /* nothing */ } while ((0))
#endif /* defined(CONFIG_PRINTK) || defined(CONFIG_STDOUT_CONSOLE) */

#ifdef CONFIG_ARC_INIT
struct shared_mem_{
	unsigned int arc_start;
};

#define SHARED_DATA ((volatile struct shared_mem_ *) VIRT_ADDR_START)
#define SCSS_REG_VAL(offset) \
	(*((volatile uint32_t *)(SCSS_REGISTER_BASE+offset)))

/**
 *
 * @brief ARC Init
 *
 * This routine initialize the ARC reset vector and
 * starts the ARC processor.
 * @return N/A
 */
static void arc_init()
{
	unsigned int *reset_vector;
	reset_vector = (unsigned int*)(RESET_VECTOR+1024);
	SCSS_REG_VAL(SCSS_SS_CFG) |= ARC_HALT_REQ_A;
	SHARED_DATA->arc_start = *reset_vector;
#ifndef CONFIG_ARC_INIT_DEBUG
	SCSS_REG_VAL(SCSS_SS_CFG) |= ARC_RUN_REQ_A;
#endif /*CONFIG_ARC_INIT_DEBUG*/
}
#endif /*CONFIG_ARC_INIT*/

/**
 *
 * @brief perform basic hardware initialization
 *
 * Initialize the Intel LOAPIC and IOAPIC device driver and the
 * Intel 8250 UART device driver.
 * Also initialize the timer device driver, if required.
 *
 * RETURNS: N/A
 */
static int qrk_se_init(struct sys_device *arg)
{
	ARG_UNUSED(arg);

	_loapic_init();
	_ioapic_init();

	consoleInit(); /* NOP if not needed */

#ifdef CONFIG_ARC_INIT
	arc_init();
#endif /*CONFIG_ARC_INIT*/

	return 0;
}
DECLARE_DEVICE_INIT_CONFIG(qrk_se_0, "", qrk_se_init, NULL);
pure_early_init(qrk_se_0, NULL);


