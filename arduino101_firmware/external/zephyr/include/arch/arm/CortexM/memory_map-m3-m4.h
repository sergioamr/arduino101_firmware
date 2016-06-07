/* memory_map-m3-m4.h - ARM CORTEX-M3/M4 memory map */

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
This module contains definitions for the memory map parts specific to the
CORTEX-M3/M4 series of processors. It is included by nanokernel/ARM/memory_map.h

 */

#ifndef _MEMORY_MAP_M3_M4__H_
#define _MEMORY_MAP_M3_M4__H_

/* 0xe0000000 -> 0xe00fffff: private peripheral bus */

/* 0xe0000000 -> 0xe003ffff: internal [256KB] */
#define _PPB_INT_BASE_ADDR (_EDEV_END_ADDR + 1)
#define _PPB_INT_ITM _PPB_INT_BASE_ADDR
#define _PPB_INT_DWT (_PPB_INT_ITM + KB(4))
#define _PPB_INT_FPB (_PPB_INT_DWT + KB(4))
#define _PPB_INT_RSVD_1 (_PPB_INT_FPB + KB(4))
#define _PPB_INT_SCS (_PPB_INT_RSVD_1 + KB(44))
#define _PPB_INT_RSVD_2 (_PPB_INT_SCS + KB(4))
#define _PPB_INT_END_ADDR (_PPB_INT_RSVD_2 + KB(196) - 1)

/* 0xe0040000 -> 0xe00fffff: external [768K] */
#define _PPB_EXT_BASE_ADDR (_PPB_INT_END_ADDR + 1)
#define _PPB_EXT_TPIU _PPB_EXT_BASE_ADDR
#define _PPB_EXT_ETM (_PPB_EXT_TPIU + KB(4))
#define _PPB_EXT_PPB (_PPB_EXT_ETM + KB(4))
#define _PPB_EXT_ROM_TABLE (_PPB_EXT_PPB + KB(756))
#define _PPB_EXT_END_ADDR (_PPB_EXT_ROM_TABLE + KB(4) - 1)

/* 0xe0100000 -> 0xffffffff: vendor-specific [0.5GB-1MB or 511MB]  */
#define _VENDOR_BASE_ADDR (_PPB_EXT_END_ADDR + 1)
#define _VENDOR_END_ADDR 0xffffffff

#endif /* _MEMORY_MAP_M3_M4__H_ */
