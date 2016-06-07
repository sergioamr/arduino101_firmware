/* pci_interface.c - PCI bus support */

/*
 * Copyright (c) 2009-2011, 2013-2014 Wind River Systems, Inc.
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

This module implements the PCI H/W access functions.

 */

#include <nanokernel.h>
#include <arch/cpu.h>

#include <pci/pci_mgr.h>
#include <string.h>
#include <board.h>

#if (PCI_CTRL_ADDR_REG == 0)
#error "PCI_CTRL_ADDR_REG cannot be zero"
#endif

#if (PCI_CTRL_DATA_REG == 0)
#error "PCI_CTRL_DATA_REG cannot be zero"
#endif

/**
 *
 * @brief Read a PCI controller register
 *
 * This routine reads the specified register from the PCI controller and
 * places the data into the provided buffer.
 *
 * @return N/A
 *
 */

static void pci_ctrl_read(uint32_t reg,   /* PCI register to read */
		    uint32_t *data, /* where to put the data */
		    uint32_t size /* size of the data to read (8/16/32 bits) */
		    )
{
	/* read based on the size requested */

	switch (size) {
		/* long (32 bits) */
	case SYS_PCI_ACCESS_32BIT:
		*data = PLB_LONG_REG_READ(reg);
		break;
		/* word (16 bits) */
	case SYS_PCI_ACCESS_16BIT:
		*data = PLB_WORD_REG_READ(reg);
		break;
		/* byte (8 bits) */
	case SYS_PCI_ACCESS_8BIT:
		*data = PLB_BYTE_REG_READ(reg);
		break;
	}
}

/**
 *
 * @brief Write a PCI controller register
 *
 * This routine writes the provided data to the specified register in the PCI
 * controller.
 *
 * @return N/A
 *
 */

static void pci_ctrl_write(uint32_t reg,  /* PCI register to write */
		     uint32_t data, /* data to write */
		     uint32_t size  /* size of the data to write (8/16/32 bits)
				      */
		     )
{
	/* write based on the size requested */

	switch (size) {
		/* long (32 bits) */
	case SYS_PCI_ACCESS_32BIT:
		PLB_LONG_REG_WRITE(data, reg);
		break;
		/* word (16 bits) */
	case SYS_PCI_ACCESS_16BIT:
		PLB_WORD_REG_WRITE(data, reg);
		break;
		/* byte (8 bits) */
	case SYS_PCI_ACCESS_8BIT:
		PLB_BYTE_REG_WRITE(data, reg);
		break;
	}
}

/**
 *
 * @brief Read the PCI controller data register
 *
 * This routine reads the data register of the specified PCI controller.
 *
 * @return 0 or -1
 *
 */

static int pci_ctrl_data_read(uint32_t controller, /* controller number */
		       uint32_t offset,     /* offset within data region */
		       uint32_t *data,      /* returned data */
		       uint32_t size	/* size of data to read */
		       )
{
	/* we only support one controller */

	if (controller != DEFAULT_PCI_CONTROLLER)
		return (-1);

	pci_ctrl_read(PCI_CTRL_DATA_REG + offset, data, size);

	return 0;
}

/**
 *
 * @brief Write the PCI controller data register
 *
 * This routine writes the provided data to the data register of the
 * specified PCI controller.
 *
 * @return 0 or -1
 *
 */

static int pci_ctrl_data_write(uint32_t controller, /* controller number */
			uint32_t offset, /* offset within address register */
			uint32_t data,   /* data to write */
			uint32_t size    /* size of data */
			)
{
	/* we only support one controller */

	if (controller != DEFAULT_PCI_CONTROLLER)
		return (-1);

	pci_ctrl_write(PCI_CTRL_DATA_REG + offset, data, size);

	return 0;
}

/**
 *
 * @brief Write the PCI controller address register
 *
 * This routine writes the provided data to the address register of the
 * specified PCI controller.
 *
 * @return 0 or -1
 *
 */

static int pci_ctrl_addr_write(uint32_t controller, /* controller number */
			uint32_t offset, /* offset within address register */
			uint32_t data,   /* data to write */
			uint32_t size    /* size of data */
			)
{
	/* we only support one controller */

	if (controller != DEFAULT_PCI_CONTROLLER)
		return (-1);

	pci_ctrl_write(PCI_CTRL_ADDR_REG + offset, data, size);
	return 0;
}

/**
 *
 * @brief Read a PCI register from a device
 *
 * This routine reads data from a PCI device's configuration space.  The
 * device and register to read is specified by the address parameter ("addr")
 * and must be set appropriately by the caller.  The address is defined by
 * the structure type pci_addr_t and contains the following members:
 *
 *     bus:     PCI bus number (0-255)
 *     device:  PCI device number (0-31)
 *     func:    device function number (0-7)
 *     reg:     device 32-bit register number to read (0-63)
 *     offset:  offset within 32-bit register to read (0-3)
 *
 * The size parameter specifies the number of bytes to read from the PCI
 * configuration space, valid values are 1, 2, and 4 bytes.  A 32-bit value
 * is always returned but it will contain only the number of bytes specified
 * by the size parameter.
 *
 * If multiple PCI controllers are present in the system, the controller id
 * can be specified in the "controller" parameter.  If only one controller
 * is present, the id DEFAULT_PCI_CONTROLLER can be used to denote this
 * controller.
 *
 * Example:
 *
 *  union pci_addr_reg addr;
 *  uint32_t status;
 *
 *  addr.field.bus    = 0;    /@ PCI bus zero        @/
 *  addr.field.device = 1;    /@ PCI device one      @/
 *  addr.field.func   = 0;    /@ PCI function zero   @/
 *  addr.field.reg    = 4;    /@ PCI register 4      @/
 *  addr.field.offset = 0;    /@ PCI register offset @/
 *
 *  pci_read (DEFAULT_PCI_CONTROLLER, addr, sizeof(uint16_t), &status);
 *
 *
 * NOTE:
 *   Reading of PCI data must be performed as an atomic operation. It is up to
 *   the caller to enforce this.
 *
 * @return N/A
 *
 */

void pci_read(uint32_t controller, /* PCI controller to use */
	     union pci_addr_reg addr,       /* PCI address to read   */
	     uint32_t size,       /* size of data in bytes */
	     uint32_t *data       /* data read from device */
	     )
{
	uint32_t access_size;
	uint32_t access_offset;

	/* validate the access size */

	switch (size) {
	case 1:
		access_size = SYS_PCI_ACCESS_8BIT;
		access_offset = addr.field.offset;
		break;
	case 2:
		access_size = SYS_PCI_ACCESS_16BIT;
		access_offset = addr.field.offset;
		break;
	case 4:
	default:
		access_size = SYS_PCI_ACCESS_32BIT;
		access_offset = 0;
		break;
	}

	/* ensure enable has been set */

	addr.field.enable = 1;

	/* clear the offset for the address register */

	addr.field.offset = 0;

	/* read the data from the PCI controller */

	pci_ctrl_addr_write(
		controller, PCI_NO_OFFSET, addr.value, SYS_PCI_ACCESS_32BIT);

	pci_ctrl_data_read(controller, access_offset, data, access_size);
}

/**
 *
 * @brief Write a to a PCI register
 *
 * This routine writes data to a PCI device's configuration space.  The
 * device and register to write is specified by the address parameter ("addr")
 * and must be set appropriately by the caller.  The address is defined by
 * the structure type pci_addr_t and contains the following members:
 *
 *     bus:     PCI bus number (0-255)
 *     device:  PCI device number (0-31)
 *     func:    device function number (0-7)
 *     reg:     device register number to read (0-63)
 *     offset:  offset within 32-bit register to write (0-3)
 *
 * The size parameter specifies the number of bytes to write to the PCI
 * configuration space, valid values are 1, 2, and 4 bytes.  A 32-bit value
 * is always provided but only the number of bytes specified by the size
 * parameter will be written to the device.
 *
 * If multiple PCI controllers are present in the system, the controller id
 * can be specified in the "controller" parameter.  If only one controller
 * is present, the id DEFAULT_PCI_CONTROLLER can be used to denote this
 * controller.
 *
 * Example:
 *
 *  pci_addr_t addr;
 *  uint32_t bar0 = 0xE0000000;
 *
 *  addr.field.bus    = 0;    /@ PCI bus zero        @/
 *  addr.field.device = 1;    /@ PCI device one      @/
 *  addr.field.func   = 0;    /@ PCI function zero   @/
 *  addr.field.reg    = 16;   /@ PCI register 16     @/
 *  addr.field.offset = 0;    /@ PCI register offset @/
 *
 *  pci_write (DEFAULT_PCI_CONTROLLER, addr, sizeof(uint32_t), bar0);
 *
 * NOTE:
 *   Writing of PCI data must be performed as an atomic operation. It is up to
 *   the caller to enforce this.
 *
 *
 * @return N/A
 *
 */

void pci_write(uint32_t controller, /* controller to use   */
	      union pci_addr_reg addr,       /* PCI address to read */
	      uint32_t size,       /* size in bytes       */
	      uint32_t data	/* data to write       */
	      )
{
	uint32_t access_size;
	uint32_t access_offset;

	/* validate the access size */

	switch (size) {
	case 1:
		access_size = SYS_PCI_ACCESS_8BIT;
		access_offset = addr.field.offset;
		break;
	case 2:
		access_size = SYS_PCI_ACCESS_16BIT;
		access_offset = addr.field.offset;
		break;
	case 4:
	default:
		access_size = SYS_PCI_ACCESS_32BIT;
		access_offset = 0;
		break;
	}

	/* ensure enable has been set */

	addr.field.enable = 1;

	/* clear the offset for the address register */

	addr.field.offset = 0;

	/* write the data to the PCI controller */

	pci_ctrl_addr_write(
		controller, PCI_NO_OFFSET, addr.value, SYS_PCI_ACCESS_32BIT);
	pci_ctrl_data_write(controller, access_offset, data, access_size);
}

/**
 *
 * @brief Get the PCI header for a device
 *
 * This routine reads the PCI header for the specified device and puts the
 * result in the supplied header structure.
 *
 * @return N/A
 */

void pci_header_get(uint32_t controller,
			union pci_addr_reg pci_ctrl_addr,
			union pci_dev *pci_dev_header)
{
	uint32_t i;

	/* clear out the header */

	memset((char *)pci_dev_header, sizeof(union pci_dev), 0);

	/* fill in the PCI header from the device */

	for (i = 0; i < PCI_HEADER_WORDS; i++) {
		pci_ctrl_addr.field.reg = i;
		pci_read(controller,
			pci_ctrl_addr,
			sizeof(uint32_t),
			&pci_dev_header->words.word[i]);
	}
}

