/* board.h - board configuration macros for the fsl_frdm_k64f BSP */

/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
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
This header file is used to specify and describe board-level aspects for the
'fsl_frdm_k64f' BSP.
 */

#ifndef _BOARD__H_
#define _BOARD__H_

#include <misc/util.h>

#define SRAM_SIZE KB(192) /* 64K in code space (ignored) + 192K in SRAM space \
			     */

/* default system clock */

#define SYSCLK_DEFAULT_IOSC_HZ MHZ(120)

/* address bases */

#define PERIPH_ADDR_BASE_MPU 0x4000D000 /* Memory Protection Unit */

#define PERIPH_ADDR_BASE_PCR 0x40049000 /* Port and pin Configuration */

#define PERIPH_ADDR_BASE_SIM 0x40047000 /* System Integration module */

#define PERIPH_ADDR_BASE_WDOG 0x40052000 /* Watchdog Timer module */

#define PERIPH_ADDR_BASE_MCG 0x40064000 /* Multipurpose Clock Generator */

#define PERIPH_ADDR_BASE_OSC 0x40065000 /* Oscillator module */

#define PERIPH_ADDR_BASE_UART0 0x4006A000 /* UART0 connected to SDA USB */
#define PERIPH_ADDR_BASE_UART1 0x4006B000 /* not supported */
#define PERIPH_ADDR_BASE_UART2 0x4006C000 /* not supported */
#define PERIPH_ADDR_BASE_UART3 0x4006D000 /* not supported */
#define PERIPH_ADDR_BASE_UART4 0x400EA000 /* not supported - for Bluetooth */

#define PERIPH_ADDR_BASE_PMC 0x4007D000 /* Power Mgt Controller module */

/* IRQs */

#define IRQ_DMA_CHAN0 0
#define IRQ_DMA_CHAN1 1
#define IRQ_DMA_CHAN2 2
#define IRQ_DMA_CHAN3 3
#define IRQ_DMA_CHAN4 4
#define IRQ_DMA_CHAN5 5
#define IRQ_DMA_CHAN6 6
#define IRQ_DMA_CHAN7 7
#define IRQ_DMA_CHAN8 8
#define IRQ_DMA_CHAN9 9
#define IRQ_DMA_CHAN10 10
#define IRQ_DMA_CHAN11 11
#define IRQ_DMA_CHAN12 12
#define IRQ_DMA_CHAN13 13
#define IRQ_DMA_CHAN14 14
#define IRQ_DMA_CHAN15 15
#define IRQ_DMA_ERR 16
#define IRQ_MCM 17
#define IRQ_FLASH_CMD 18
#define IRQ_FLASH_COLLISION 19
#define IRQ_LOW_VOLTAGE 20
#define IRQ_LOW_LEAKAGE 21
#define IRQ_WDOG_OR_EVM 22
#define IRQ_RAND_NUM_GEN 23
#define IRQ_I2C0 24
#define IRQ_I2C1 25
#define IRQ_SPI0 26
#define IRQ_SPI1 27
#define IRQ_I2S0_TX 28
#define IRQ_I2S0_RX 29
#define IRQ_RESERVED0 30
#define IRQ_UART0_STATUS 31
#define IRQ_UART0_ERROR 32
#define IRQ_UART1_STATUS 33
#define IRQ_UART1_ERROR 34
#define IRQ_UART2_STATUS 35
#define IRQ_UART2_ERROR 36
#define IRQ_UART3_STATUS 37
#define IRQ_UART3_ERROR 38
#define IRQ_ADC0 39
#define IRQ_CMP0 40
#define IRQ_CMP1 41
#define IRQ_FTM0 42
#define IRQ_FTM1 43
#define IRQ_FTM2 44
#define IRQ_CMT 45
#define IRQ_RTC_ALARM 46
#define IRQ_RTC_SEC 47
#define IRQ_TIMER0 48
#define IRQ_TIMER1 49
#define IRQ_TIMER2 50
#define IRQ_TIMER3 51
#define IRQ_PDB 52
#define IRQ_USB_OTG 53
#define IRQ_USB_CHARGE 54
#define IRQ_RESERVED1 55
#define IRQ_DAC0 56
#define IRQ_MCG 57
#define IRQ_LOW_PWR_TIMER 58
#define IRQ_GPIO_PORTA 59
#define IRQ_GPIO_PORTB 60
#define IRQ_GPIO_PORTC 61
#define IRQ_GPIO_PORTD 62
#define IRQ_GPIO_PORTE 63
#define IRQ_SOFTWARE 64
#define IRQ_SPI2 65
#define IRQ_UART4_STATUS 66
#define IRQ_UART4_ERROR 67
#define IRQ_RESERVED2 68 /* IRQ_UART5_STATUS - UART5 not implemented */
#define IRQ_RESERVED3 69 /* IRQ_UART5_ERROR - UART5 not implemented */
#define IRQ_CMP2 70
#define IRQ_FTM3 71
#define IRQ_DAC1 72
#define IRQ_ADC1 73
#define IRQ_I2C2 74
#define IRQ_CAN0_MSG_BUF 75
#define IRQ_CAN0_BUS_OFF 76
#define IRQ_CAN0_ERROR 77
#define IRQ_CAN0_TX_WARN 78
#define IRQ_CAN0_RX_WARN 79
#define IRQ_CAN0_WAKEUP 80
#define IRQ_SDHC 81
#define IRQ_ETH_IEEE1588_TMR 82
#define IRQ_ETH_TX 83
#define IRQ_ETH_RX 84
#define IRQ_ETH_ERR_MISC 85

#ifndef _ASMLANGUAGE

#include <misc/util.h>
#include <drivers/rand32.h>

/*
 * UART configuration settings
 */

#include <drivers/k20_pcr.h>

#define CONFIG_UART_NUM_SYSTEM_PORTS 5
#define CONFIG_UART_NUM_EXTRA_PORTS 0
#define CONFIG_UART_NUM_PORTS \
	(CONFIG_UART_NUM_SYSTEM_PORTS + CONFIG_UART_NUM_EXTRA_PORTS)

#define CONFIG_UART_PORT_0_REGS PERIPH_ADDR_BASE_UART0
#define CONFIG_UART_PORT_0_IRQ IRQ_UART0_STATUS
#define CONFIG_UART_PORT_1_REGS PERIPH_ADDR_BASE_UART1
#define CONFIG_UART_PORT_1_IRQ IRQ_UART1_STATUS
#define CONFIG_UART_PORT_2_REGS PERIPH_ADDR_BASE_UART2
#define CONFIG_UART_PORT_2_IRQ IRQ_UART2_STATUS
#define CONFIG_UART_PORT_3_REGS PERIPH_ADDR_BASE_UART3
#define CONFIG_UART_PORT_3_IRQ IRQ_UART3_STATUS
#define CONFIG_UART_PORT_4_REGS PERIPH_ADDR_BASE_UART4
#define CONFIG_UART_PORT_4_IRQ IRQ_UART4_STATUS

#define UART_PORTS_CONFIGURE(__type, __name)				\
	static __type __name[CONFIG_UART_NUM_PORTS] = {			\
		{							\
			.base = (uint8_t *)CONFIG_UART_PORT_0_REGS,	\
			.irq = CONFIG_UART_PORT_0_IRQ			\
		},							\
		{							\
			.base = (uint8_t *)CONFIG_UART_PORT_1_REGS,	\
			.irq = CONFIG_UART_PORT_1_IRQ			\
		},							\
		{							\
			.base = (uint8_t *)CONFIG_UART_PORT_2_REGS,	\
			.irq = CONFIG_UART_PORT_2_IRQ			\
		},							\
		{							\
			.base = (uint8_t *)CONFIG_UART_PORT_3_REGS,	\
			.irq = CONFIG_UART_PORT_3_IRQ			\
		},							\
		{							\
			.base = (uint8_t *)CONFIG_UART_PORT_4_REGS,	\
			.irq = CONFIG_UART_PORT_4_IRQ			\
		}							\
	}


/* Uart console settings */

#define CONFIG_UART_CONSOLE_PORT PCR_PORT_B
#define CONFIG_UART_CONSOLE_PORT_RX_PIN 16
#define CONFIG_UART_CONSOLE_PORT_TX_PIN 17
#define CONFIG_UART_CONSOLE_PORT_MUX_FUNC PCR_MUX_ALT3
#define CONFIG_UART_CONSOLE_CLK_FREQ SYSCLK_DEFAULT_IOSC_HZ
#define CONFIG_UART_CONSOLE_BAUDRATE 115200
#define CONFIG_UART_CONSOLE_IRQ IRQ_UART0_STATUS
#define CONFIG_UART_CONSOLE_INT_PRI 3

/* Bluetooth UART definitions */
#define CONFIG_BLUETOOTH_UART_INDEX 1
#define CONFIG_BLUETOOTH_UART_BAUDRATE 115200
#define CONFIG_BLUETOOTH_UART_IRQ IRQ_UART1_STATUS
#define CONFIG_BLUETOOTH_UART_INT_PRI 3
#define CONFIG_BLUETOOTH_UART_FREQ SYSCLK_DEFAULT_IOSC_HZ

#define EXC_FROM_IRQ(irq) ((irq) + 16)
#define VECTOR_FROM_IRQ(irq) EXC_FROM_IRQ(irq)
#define VECTOR_ADDR(vector) ((uint32_t *)((int)vector << 2))

/*
 * Device drivers utilize the macros PLB_BYTE_REG_WRITE() and
 * PLB_BYTE_REG_READ() to access byte-wide registers on the processor
 * local bus (PLB), as opposed to a PCI bus, for example.  Boards are
 * expected to provide implementations of these macros.
 */

static inline void __plbByteRegWrite(unsigned char data, unsigned char *pAddr)
{
	*pAddr = data;
}

#define PLB_BYTE_REG_WRITE(data, address) \
	__plbByteRegWrite((unsigned char)data, (unsigned char *)address)

static inline unsigned char __plbByteRegRead(unsigned char *pAddr)
{
	return *pAddr;
}

#define PLB_BYTE_REG_READ(address) __plbByteRegRead((unsigned char *)address)

#endif /* !_ASMLANGUAGE */

#endif /* _BOARD__H_ */
