/* command_packet.h - command packet header file */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
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

#ifndef _COMMAND_PACKET_H
#define _COMMAND_PACKET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <microkernel/base_api.h>

/* define size of command packet (without exposing its internal structure) */

#define CMD_PKT_SIZE_IN_WORDS (19)

/**
 *
 * @brief Define an instance of a command packet set
 *
 * This macro is used to create an instance of a command packet set in the
 * global namespace.  Each instance of the set may have its own unique number
 * of command packets.
 *
 * INTERNAL
 * It is critical that the word corresponding to the [alloc] field in the
 * equivalent struct k_args command packet be zero so that the system knows that the
 * command packet is not part of the free list.
 */

#define CMD_PKT_SET_INSTANCE(name, num) \
	uint32_t name[2 + CMD_PKT_SIZE_IN_WORDS * (num)] = {num, 0}

/**
 *
 * @brief Wrapper for accessing a command packet set
 *
 * As a command packet set is instantiated as an array of uint32_t, it is
 * necessary to typecast a command packet set before accessing it.
 */

#define CMD_PKT_SET(name) (*(struct cmd_pkt_set *)(name))

typedef uint32_t cmdPkt_t[CMD_PKT_SIZE_IN_WORDS];

struct cmd_pkt_set {
	uint32_t nPkts;    /* number of command packets in set */
	uint32_t index;    /* index into command packet array */
	cmdPkt_t cmdPkt[]; /* array of command packets */
};

/* externs */

extern cmdPkt_t *_cmd_pkt_get(struct cmd_pkt_set *pSet);

#ifdef __cplusplus
}
#endif

#endif /* _COMMAND_PACKET_H */
