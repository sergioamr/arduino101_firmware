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
/* dummy_15_4_radio.c - 802.15.4 radio driver loopbacks Tx frames back to us */


#include "contiki.h"

#include <net/net_buf.h>
#include <simple/uart.h>

#include "net/packetbuf.h"
#include "net/netstack.h"
#include "dummy_15_4_radio.h"
#include "net_driver_15_4.h"

#include <string.h>

#if UIP_CONF_LOGGING
#define DEBUG DEBUG_FULL
#else
#define DEBUG DEBUG_NONE
#endif
#include "net/ip/uip-debug.h"

#if UIP_LOGGING
#include <stdio.h>
void uip_log(char *msg);
#define UIP_LOG(m) uip_log(m)
#else
#define UIP_LOG(m)
#endif

#define FOOTER_LEN 2
#define NETWORK_TEST_MAX_PACKET_LEN      PACKETBUF_SIZE

static volatile uint16_t last_packet_timestamp;

/* Data sending and receiving is done in TLV way. */
#if defined CONFIG_NETWORKING_WITH_15_4_LOOPBACK_UART
#define DUMMY_RADIO_15_4_FRAME_TYPE	0xF0
static uint8_t input[NETWORK_TEST_MAX_PACKET_LEN];
static uint8_t input_len, input_offset;
#else
static uint8_t loopback[NETWORK_TEST_MAX_PACKET_LEN];
#endif

/*---------------------------------------------------------------------------*/
#if defined CONFIG_NETWORKING_WITH_15_4_LOOPBACK_UART
static uint8_t *recv_cb(uint8_t *buf, size_t *off)
{
  if (input_len == 0 && input_offset == 0 &&
       buf[0] == DUMMY_RADIO_15_4_FRAME_TYPE) {
    goto done;
  }

  if (input_len == 0 && input_offset == 0 &&
       buf[0] != DUMMY_RADIO_15_4_FRAME_TYPE) {
    input_len = buf[0];
    goto done;
  }

  if (input_len) {
    input[input_offset++] = buf[0];
  }

  if (input_len && input_len == input_offset) {
     struct net_mbuf *mbuf;
     mbuf = net_mbuf_get_reserve(0);
     if (mbuf) {
       packetbuf_copyfrom(mbuf, input, input_len);
       packetbuf_set_datalen(mbuf, input_len);
       packetbuf_set_attr(mbuf, PACKETBUF_ATTR_TIMESTAMP, last_packet_timestamp);
       PRINTF("dummy154radio: received %d bytes\n", input_len);

       if (net_driver_15_4_recv_from_hw(mbuf) < 0) {
         PRINTF("dummy154radio: rdc input failed, packet discarded\n");
         net_mbuf_put(mbuf);
       }
     }

     input_len = input_offset = 0;
     memset(input, 0, sizeof(input));
  }

done:
  *off = 0;
  return buf;
}
#endif

/*---------------------------------------------------------------------------*/
static int
init(void)
{
#if defined CONFIG_NETWORKING_WITH_15_4_LOOPBACK_UART
  /* Use small temp buffer for receiving data */
  static uint8_t buf[1];

  uart_simple_register(buf, sizeof(buf), recv_cb);
#endif

  return 0;
}
/*---------------------------------------------------------------------------*/
static int
prepare(const void *payload, unsigned short payload_len)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
transmit(struct net_mbuf *buf, unsigned short transmit_len)
{
  return RADIO_TX_OK;
}

#ifndef CONFIG_NETWORKING_WITH_15_4_LOOPBACK_UART
static void route_buf(struct net_mbuf *buf)
{
	int len;
	struct net_mbuf *mbuf;

	len = packetbuf_copyto(buf, loopback);
	/* Receiver buffer that is passed to 15.4 Rx fiber */
	PRINTF("dummy154radio: got %d bytes\n", len);

	mbuf = net_mbuf_get_reserve(0);
	if (mbuf) {
		packetbuf_copyfrom(mbuf, loopback, len);
		packetbuf_set_datalen(mbuf, len);
		packetbuf_set_attr(mbuf, PACKETBUF_ATTR_TIMESTAMP,
						last_packet_timestamp);
		PRINTF("dummy154radio: 15.4 Rx input %d bytes\n", len);

		if (net_driver_15_4_recv_from_hw(mbuf) < 0) {
			PRINTF("dummy154radio: rdc input failed, "
							"packet discarded\n");
			net_mbuf_put(mbuf);
		}

		NET_BUF_CHECK_IF_NOT_IN_USE(mbuf);
	}
}
#endif

#if defined CONFIG_NETWORKING_WITH_15_4_LOOPBACK_UART
static int uart_send(unsigned char c)
{
  uint8_t buf[1] = { c };

  return uart_simple_send(&buf[0], 1);
}
#endif

/*---------------------------------------------------------------------------*/
static int
send(struct net_mbuf *buf, const void *payload, unsigned short payload_len)
{
#if defined CONFIG_NETWORKING_WITH_15_4_LOOPBACK_UART
  static uint8_t output[NETWORK_TEST_MAX_PACKET_LEN];
  uint8_t len, i;

  len = packetbuf_copyto(buf, output);
  uart_send(DUMMY_RADIO_15_4_FRAME_TYPE); /* Type */
  uart_send(len);  /* Length */

  PRINTF("dummy154radio: sending %d bytes\n", len);

  for (i = 0; i < len; i++) {
    if (!uart_send(output[i])) {
      PRINTF("uart_send failed\n");
      return RADIO_TX_ERR;
    }
  }

  return RADIO_TX_OK;
#else
  route_buf(buf);
  return transmit(buf, payload_len);
#endif
}
/*---------------------------------------------------------------------------*/
static int
radio_read(void *buf, unsigned short buf_len)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
channel_clear(void)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
receiving_packet(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
pending_packet(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
off(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
get_value(radio_param_t param, radio_value_t *value)
{
  return RADIO_RESULT_NOT_SUPPORTED;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
set_value(radio_param_t param, radio_value_t value)
{
  return RADIO_RESULT_NOT_SUPPORTED;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
get_object(radio_param_t param, void *dest, size_t size)
{
  return RADIO_RESULT_NOT_SUPPORTED;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
set_object(radio_param_t param, const void *src, size_t size)
{
  return RADIO_RESULT_NOT_SUPPORTED;
}
/*---------------------------------------------------------------------------*/
const struct radio_driver dummy154radio_driver =
  {
    .init = init,
    .prepare = prepare,
    .transmit = transmit,
    .send = send,
    .read = radio_read,
    .channel_clear = channel_clear,
    .receiving_packet = receiving_packet,
    .pending_packet = pending_packet,
    .on = on,
    .off = off,
    .get_value = get_value,
    .set_value = set_value,
    .get_object = get_object,
    .set_object = set_object
  };
/*---------------------------------------------------------------------------*/
