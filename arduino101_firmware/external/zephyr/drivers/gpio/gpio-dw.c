/*
 * Copyright (c) 2016, Intel Corporation
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
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

#include <nanokernel.h>
#include <gpio.h>
#include <gpio/gpio-dw.h>
#include <board.h>

#define SWPORTA_DR	0x00
#define SWPORTA_DDR	0x04
#define SWPORTB_DR	0x0c
#define SWPORTB_DDR	0x10
#define SWPORTC_DR	0x18
#define SWPORTC_DDR	0x1c
#define SWPORTD_DR	0x24
#define SWPORTD_DDR	0x28
#define INTEN		0x30
#define INTMASK		0x34
#define INTTYPE_LEVEL	0x38
#define INT_POLARITY	0x3c
#define INTSTATUS	0x40
#define PORTA_DEBOUNCE	0x48
#define PORTA_EOI	0x4c
#define EXT_PORTA	0x50
#define EXT_PORTB	0x54
#define EXT_PORTC	0x58
#define EXT_PORTD	0x5c
#define INT_CLOCK_SYNC	0x60
#define INT_BOTHEDGE	0x68

#define BIT(n)	(1UL << (n))

static inline uint32_t dw_read(uint32_t base_addr, uint32_t offest)
{
	return *(uint32_t*)(base_addr+offest);
}

static inline void dw_write(uint32_t base_addr, uint32_t offest,
			   uint32_t val)
{
	*(uint32_t*)(base_addr+offest) = val;
}


static void dw_set_bit(uint32_t base_addr, uint32_t offset,
			      uint32_t bit, uint8_t value)
{
	uint32_t reg;

	reg = dw_read(base_addr, offset);
	reg &= ~BIT(bit);
	reg |= ((value & 0x1) << bit);
	dw_write(base_addr, offset, reg);
}

static inline void dw_interrupt_config(struct device *port, int access_op,
			 uint32_t pin, int flags)
{
	struct gpio_config_dw *config = port->config->config_info;
	uint32_t base_addr = config->base_addr;

	uint8_t flag_is_set;

	/* set as an input pin */
	dw_set_bit(base_addr, SWPORTA_DDR, pin, 0);

	/* level or edge */
	flag_is_set = (flags & GPIO_INT_EDGE);
	dw_set_bit(base_addr, INTTYPE_LEVEL, pin, flag_is_set);

	/* Active low/high */
	flag_is_set = (flags & GPIO_INT_ACTIVE_HIGH);
	dw_set_bit(base_addr, INT_POLARITY, pin, flag_is_set);

	/* both edges */
	flag_is_set = (flags & GPIO_INT_DOUBLE_EDGE);
	if (flag_is_set) {
		dw_set_bit(base_addr, INT_BOTHEDGE, pin, flag_is_set);
		dw_set_bit(base_addr, INTTYPE_LEVEL, pin, flag_is_set);
	}

	/* use built-in debounce  */
	flag_is_set = (flags & GPIO_INT_DEBOUNCE );
	dw_set_bit(base_addr, PORTA_DEBOUNCE, pin, flag_is_set);

	/* level triggered int synchronous with clock */
	flag_is_set = (flags & GPIO_INT_CLOCK_SYNC );
	dw_set_bit(base_addr, INT_CLOCK_SYNC, pin, flag_is_set);
	dw_set_bit(base_addr, INTEN, pin, 1);
}

static inline void dw_pin_config(struct device *port,
			 uint32_t pin, int flags)
{
	struct gpio_config_dw *config = port->config->config_info;
	uint32_t base_addr = config->base_addr;

	/* clear interrupt enable */
	dw_set_bit(base_addr, INTEN, pin, 0);

	/* set direction */
	dw_set_bit(base_addr, SWPORTA_DDR, pin, (flags & GPIO_DIR_MASK));

	if (flags &  GPIO_INT)
		dw_interrupt_config(port, GPIO_ACCESS_BY_PIN, pin, flags);
}

static inline void dw_port_config(struct device *port, int flags)
{
	struct gpio_config_dw *config = port->config->config_info;
	int i;

	for (i=0; i < config->bits; i++) {
		dw_pin_config(port, i, flags);
	}
}

static inline int gpio_config_dw(struct device *port, int access_op,
				 uint32_t pin, int flags)
{
	if (((flags & GPIO_INT) && (flags & GPIO_DIR_OUT)) ||
	    ((flags & GPIO_DIR_IN) && (flags & GPIO_DIR_OUT))) {
		return -1;
	}

	if (GPIO_ACCESS_BY_PIN == access_op) {
		dw_pin_config(port, pin, flags);
	} else {
		dw_port_config(port, flags);
	}
	return 0;
}

static inline int gpio_write_dw(struct device *port, int access_op,
				uint32_t pin, uint32_t value)
{
	struct gpio_config_dw *config = port->config->config_info;
	uint32_t base_addr = config->base_addr;

	if (GPIO_ACCESS_BY_PIN == access_op) {
		dw_set_bit(base_addr, SWPORTA_DR, pin, value);
	} else {
		dw_write(base_addr, SWPORTA_DR, value);
	}

	return 0;
}

static inline int gpio_read_dw(struct device *port, int access_op,
				    uint32_t pin, uint32_t *value)
{
	struct gpio_config_dw *config = port->config->config_info;
	uint32_t base_addr = config->base_addr;

	*value = dw_read(base_addr, EXT_PORTA);

	if (GPIO_ACCESS_BY_PIN == access_op) {
		*value = !!(*value & BIT(pin));
	}
	return 0;
}

static inline int gpio_set_callback_dw(struct device *port,
				       gpio_callback_t callback)
{
	struct gpio_runtime_dw *context = port->driver_data;

	context->callback = callback;

	return 0;
}

static inline int gpio_enable_callback_dw(struct device *port, int access_op,
					  uint32_t pin)
{
	struct gpio_config_dw *config = port->config->config_info;
	struct gpio_runtime_dw *context = port->driver_data;
	uint32_t base_addr = config->base_addr;

	if (GPIO_ACCESS_BY_PIN == access_op) {
		context->enabled_callbacks |= BIT(pin);
	} else {
		context->port_callback = 1;
	}
	dw_write(base_addr, PORTA_EOI, BIT(pin));
	dw_set_bit(base_addr, INTMASK, pin, 0);
	return 0;
}

static inline int gpio_disable_callback_dw(struct device *port, int access_op,
					   uint32_t pin)
{
	struct gpio_config_dw *config = port->config->config_info;
	struct gpio_runtime_dw *context = port->driver_data;
	uint32_t base_addr = config->base_addr;

	if (GPIO_ACCESS_BY_PIN == access_op) {
		context->enabled_callbacks &= ~(BIT(pin));
	} else {
		context->port_callback = 0;
	}
	dw_set_bit(base_addr, INTMASK, pin, 1);

	return 0;
}

static inline int gpio_suspend_port_dw(struct device *port)
{
	return 0;
}

static inline int gpio_resume_port_dw(struct device *port)
{
	return 0;
}

#ifdef CONFIG_BSP_QUARK_SE_X86
#define GPIO_CLKENA_POS (31)
#define LS_SYNC_POS (0)
/* The quark_se SOC has a clock enable bit in the INT_CLOCK_SYNC
 * register which only has bit 0 as valid in the synopsys
 * documentation. These functions handle this bit if we are being
 * built for the Quark_SE SOC
 */
static inline void gpio_enable_clock_dw(uint32_t base_addr)
{
	dw_set_bit(base_addr, INT_CLOCK_SYNC, GPIO_CLKENA_POS, 1);
	dw_set_bit(base_addr, INT_CLOCK_SYNC, LS_SYNC_POS, 1);
}

static inline void gpio_disable_clock_dw(uint32_t base_addr)
{
	dw_set_bit(base_addr, INT_CLOCK_SYNC, GPIO_CLKENA_POS, 0);
	dw_set_bit(base_addr, INT_CLOCK_SYNC, LS_SYNC_POS, 0);
}

static inline void gpio_unmask_int_dw(uint32_t irq)
{
	SCSS_INTERRUPT->int_gpio_mask &= INT_UNMASK_IA;
}
#else
static inline void gpio_enable_clock_dw(uint32_t base_addr) {}
static inline void gpio_disable_clock_dw(uint32_t base_addr) {}
static inline void gpio_unmask_int_dw(uint32_t irq) {}
#endif



void gpio_dw_isr(struct device *port)
{
	struct gpio_runtime_dw *context = port->driver_data;
	struct gpio_config_dw *config = port->config->config_info;
	uint32_t base_addr = config->base_addr;
	uint32_t int_status, bit;

	int_status = dw_read(base_addr, INTSTATUS);
	dw_write(base_addr, PORTA_EOI, -1);

	if (!context->callback) {
		return;
	}
	if (context->port_callback) {
		context->callback(port, int_status);
		return;
	}

	if (context->enabled_callbacks) {
		for (bit = 0; bit < 32; bit++) {
			if (context->enabled_callbacks & (1 << bit)) {
				context->callback(port, (1 << bit));
			}
		}
	}

}

static struct gpio_driver_api api_funcs = {
	.config = gpio_config_dw,
	.write = gpio_write_dw,
	.read = gpio_read_dw,
	.set_callback = gpio_set_callback_dw,
	.enable_callback = gpio_enable_callback_dw,
	.disable_callback = gpio_disable_callback_dw,
	.suspend = gpio_suspend_port_dw,
	.resume = gpio_resume_port_dw
};

int gpio_initialize_dw(struct device *port)
{
	struct gpio_config_dw *config = port->config->config_info;
	uint32_t base_addr = config->base_addr;

	gpio_enable_clock_dw(base_addr);

	/* interrupts in sync with system clock */
	dw_set_bit(base_addr, INT_CLOCK_SYNC, 0, 1);

	/* mask and disable interrupts */
	dw_write(base_addr, INTMASK, ~(0));
	dw_write(base_addr, INTEN, 0);
	dw_write(base_addr, PORTA_EOI, ~(0));

	port->driver_api = &api_funcs;

	config->config_func(port);
	gpio_unmask_int_dw(config->irq_num);
	irq_enable(config->irq_num);

	return 0;
}
