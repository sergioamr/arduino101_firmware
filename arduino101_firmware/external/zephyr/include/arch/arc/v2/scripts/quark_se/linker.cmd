/* linker.cmd - Linker command/script file */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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
Linker script for the Quark_SE ARC BSPs.
*/

#define _LINKER
#define _ASMLANGUAGE

#include <autoconf.h>
#include <sections.h>

#if defined(CONFIG_NSIM)
	#define KENTRY __reset
	EXTERN(_VectorTable)
#else
	#define KENTRY _VxMicroStart
#endif

#include <linker-tool.h>

#define INIT_LEVEL(level)				\
		__initconfig##level##_start = .;	\
		*(.initconfig##level##.init)		\

/* physical address of RAM */
#ifdef CONFIG_XIP
	#define ROMABLE_REGION FLASH
	#define RAMABLE_REGION SRAM
#else
	#define ROMABLE_REGION SRAM
	#define RAMABLE_REGION SRAM
#endif

#if defined(CONFIG_XIP)
	#define _DATA_IN_ROM __data_rom_start
#else
	#define _DATA_IN_ROM
#endif

MEMORY {
	FLASH (rx) : ORIGIN = FLASH_START, LENGTH = FLASH_SIZE
	SRAM  (wx) : ORIGIN = SRAM_START,  LENGTH = SRAM_SIZE
	DCCM  (wx) : ORIGIN = DCCM_START,  LENGTH = DCCM_SIZE
}

	__ram_phys_end = SRAM_START + SRAM_SIZE;

SECTIONS {
	GROUP_START(ROMABLE_REGION)

	SECTION_PROLOGUE(_TEXT_SECTION_NAME,,ALIGN(1024)) {
		__text_start = .;

/* when !XIP, .text is in RAM, and vector table must be at its very start */

		KEEP(*(.exc_vector_table))
		KEEP(*(".exc_vector_table.*"))


		KEEP(*(.irq_vector_table))
		KEEP(*(".irq_vector_table.*"))

#if defined(CONFIG_VERSION_HEADER)
                *(.version_header)
                KEEP(*(".version_header*"))
#endif

		*(.text)
		*(".text.*")

		__text_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE (devconfig, (OPTIONAL),)
	{
		__devconfig_start = .;
		*(".devconfig.*")
		KEEP(*(SORT_BY_NAME(".devconfig*")))
		__devconfig_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE (.pshinit, (OPTIONAL),) {
		/* This section is used to store psh_core funcs,data*/
		. = ALIGN(8);
		_s_pshinit = .;
		_s_pshinit_fn = .;
		KEEP (*(.pshinit.fn))
		_e_pshinit_fn = .;
		_s_drvinit = .;
		KEEP(*(.pshinit.drv))
		_e_drvinit = .;
		_s_options = .;
		KEEP(*(.pshinit.options))
		_e_options = .;
		KEEP (*(.pshinit.data))
		KEEP (*(.pshinit))
		_e_pshinit = .;
	} GROUP_LINK_IN(ROMABLE_REGION)
                SECTION_PROLOGUE (.openinit, (OPTIONAL),) {
		/* This section is used to store sensor funcs,data*/
                        . = ALIGN(8);
                        _s_openinit = .;

                        _s_feedinit = .;
                        KEEP (*(.openinit.feed))
                        _e_feedinit = .;

                        _s_exposedinit = .;
                        KEEP(*(.openinit.exposed))
                        _e_exposedinit = .;

                        KEEP (*(.openinit.data))
                        KEEP (*(.openinit))
                        _e_openinit = .;
                } GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE(_CTOR_SECTION_NAME,,) {
		/*
		 * The compiler fills the constructor pointers table below, hence
		 * symbol __CTOR_LIST__ must be aligned on 4 byte boundary.
		 * To align with the C++ standard, the first element of the array
		 * contains the number of actual constructors. The last element is
		 * NULL.
		 */
		. = ALIGN(4);
		__CTOR_LIST__ = .;
		LONG((__CTOR_END__ - __CTOR_LIST__) / 4 - 2)
		KEEP(*(SORT_BY_NAME(".ctors*")))
		LONG(0)
		__CTOR_END__ = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE(_RODATA_SECTION_NAME,,) {
		*(.rodata)
		*(".rodata.*")
	} GROUP_LINK_IN(ROMABLE_REGION)

#if defined(CONFIG_TEST_COMMAND)
        SECTION_PROLOGUE (tcmd_section, (OPTIONAL),)
        {
                 /* This section is used to store test commands.  */
                . = ALIGN(8);
                __test_cmds_start = .;
                KEEP(*(SORT(.test_cmds.*)))
                __test_cmds_end = .;
        } GROUP_LINK_IN(ROMABLE_REGION)
#endif
	__data_rom_start = ALIGN(4);	/* XIP imaged DATA ROM start addr */

	GROUP_END(ROMABLE_REGION)

	GROUP_START(RAMABLE_REGION)

#if defined(CONFIG_XIP)
	SECTION_AT_PROLOGUE(_DATA_SECTION_NAME,,,_DATA_IN_ROM) {
#else
	SECTION_PROLOGUE(_DATA_SECTION_NAME,,) {
#endif

/* when XIP, .text is in ROM, but vector table must be at start of .data */

		__data_ram_start = .;
		*(.data)
		*(".data.*")
	} GROUP_LINK_IN(RAMABLE_REGION)

	SECTION_PROLOGUE(initlevel, (OPTIONAL),)
	{
		__initconfig_start = .;
		INIT_LEVEL(0)
		INIT_LEVEL(1)
		INIT_LEVEL(2)
		INIT_LEVEL(3)
		INIT_LEVEL(4)
		INIT_LEVEL(5)
		INIT_LEVEL(6)
		INIT_LEVEL(7)
		KEEP(*(SORT_BY_NAME(".initconfig*")))
		__initconfig_end = .;
	} GROUP_LINK_IN(RAMABLE_REGION)


	__data_ram_end = .;

	SECTION_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),) {
		/*
		 * For performance, BSS section is assumed to be 4 byte aligned and
		 * a multiple of 4 bytes
		 */
		. = ALIGN(4);
		__bss_start = .;
		*(.bss)
		*(".bss.*")
		COMMON_SYMBOLS
		/*
		 * BSP clears this memory in words only and doesn't clear any
		 * potential left over bytes.
		 */
		__bss_end = ALIGN(4);
	} GROUP_LINK_IN(RAMABLE_REGION)

	SECTION_PROLOGUE(_NOINIT_SECTION_NAME,(NOLOAD),) {
		/*
		 * This section is used for non-initialized objects that
		 * will not be cleared during the boot process.
		 */
		*(.noinit)
		*(".noinit.*")

	} GROUP_LINK_IN(RAMABLE_REGION)

	/* Define linker symbols */

	_end = .; /* end of image */
	__bss_num_words = (__bss_end - __bss_start) >> 2;

	GROUP_END(RAMABLE_REGION)

	/* Data Closely Coupled Memory (DCCM). DCCM is now used by algorithms. */
	GROUP_START(DCCM)
	    SECTION_PROLOGUE(dccm,(NOLOAD),)
	    {
		    . = ALIGN(4);
		    __dccm_start = .;
		    *(.dccm)
			_dccm_end = ALIGN(4);
	    } GROUP_LINK_IN(DCCM)
	GROUP_END(DCCM)

	}

#ifdef CONFIG_XIP
/*
 * Round up number of words for DATA section to ensure that XIP copies the
 * entire data section. XIP copy is done in words only, so there may be up
 * to 3 extra bytes copied in next section (BSS). At run time, the XIP copy
 * is done first followed by clearing the BSS section.
 */
__data_size = (__data_ram_end - __data_ram_start);
__data_num_words = (__data_size + 3) >> 2;

#endif
