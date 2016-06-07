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
/* compression.h - Header Compression */


#include <stdio.h>
#include <string.h>

#include <net/net_buf.h>
#include <net/sicslowpan/sicslowpan_compression.h>
#include <net/netstack.h>
#include "net/packetbuf.h"
#include "net/ip/uip.h"
#include "net/ip/tcpip.h"
#include "dev/watchdog.h"
#include "contiki/ipv6/uip-ds6.h"

#define DEBUG 0
#include "net/ip/uip-debug.h"
#if DEBUG
/* PRINTFI and PRINTFO are defined for input and output to debug one without changing the timing of the other */
uint8_t p;
#include <stdio.h>
#define PRINTFI(...) PRINTF(__VA_ARGS__)
#define PRINTFO(...) PRINTF(__VA_ARGS__)
#define PRINTPACKETBUF() PRINTF("packetbuf buffer: "); for(p = 0; p < packetbuf_datalen(); p++){PRINTF("%.2X", *(packetbuf_ptr + p));} PRINTF("\n")
#define PRINTUIPBUF() PRINTF("UIP buffer: "); for(p = 0; p < uip_len; p++){PRINTF("%.2X", uip_buf[p]);}PRINTF("\n")
#define PRINTSICSLOWPANBUF() PRINTF("SICSLOWPAN buffer: "); for(p = 0; p < sicslowpan_len; p++){PRINTF("%.2X", sicslowpan_buf[p]);}PRINTF("\n")
#else
#define PRINTFI(...)
#define PRINTFO(...)
#define PRINTPACKETBUF()
#define PRINTUIPBUF()
#define PRINTSICSLOWPANBUF()
#endif /* DEBUG == 1*/

#if UIP_LOGGING
#include <stdio.h>
void uip_log(char *msg);
#define UIP_LOG(m) uip_log(m)
#else
#define UIP_LOG(m)
#endif /* UIP_LOGGING == 1 */

#ifndef SICSLOWPAN_COMPRESSION
#ifdef SICSLOWPAN_CONF_COMPRESSION
#define SICSLOWPAN_COMPRESSION SICSLOWPAN_CONF_COMPRESSION
#else
#define SICSLOWPAN_COMPRESSION SICSLOWPAN_COMPRESSION_IPV6
#endif /* SICSLOWPAN_CONF_COMPRESSION */
#endif /* SICSLOWPAN_COMPRESSION */

#define GET16(ptr,index) (((uint16_t)((ptr)[index] << 8)) | ((ptr)[(index) + 1]))
#define SET16(ptr,index,value) do {     \
  (ptr)[index] = ((value) >> 8) & 0xff; \
  (ptr)[index + 1] = (value) & 0xff;    \
} while(0)

/** \name Pointers in the packetbuf buffer
 *  @{
 */

/* define the buffer as a byte array */
#define PACKETBUF_IPHC_BUF(buf)              ((uint8_t *)(uip_packetbuf_ptr(buf) + uip_packetbuf_hdr_len(buf)))

#define PACKETBUF_HC1_PTR(buf)            (uip_packetbuf_ptr(buf) + uip_packetbuf_hdr_len(buf))
#define PACKETBUF_HC1_DISPATCH       0 /* 8 bit */
#define PACKETBUF_HC1_ENCODING       1 /* 8 bit */
#define PACKETBUF_HC1_TTL            2 /* 8 bit */

#define PACKETBUF_HC1_HC_UDP_PTR(buf)           (uip_packetbuf_ptr(buf) + uip_packetbuf_hdr_len(buf))
#define PACKETBUF_HC1_HC_UDP_DISPATCH      0 /* 8 bit */
#define PACKETBUF_HC1_HC_UDP_HC1_ENCODING  1 /* 8 bit */
#define PACKETBUF_HC1_HC_UDP_UDP_ENCODING  2 /* 8 bit */
#define PACKETBUF_HC1_HC_UDP_TTL           3 /* 8 bit */
#define PACKETBUF_HC1_HC_UDP_PORTS         4 /* 8 bit */
#define PACKETBUF_HC1_HC_UDP_CHKSUM        5 /* 16 bit */

/** \name Pointers in the sicslowpan and uip buffer
 *  @{
 */
#define SICSLOWPAN_IP_BUF(buf)   ((struct uip_ip_hdr *)&buf[UIP_LLH_LEN])
#define SICSLOWPAN_UDP_BUF(buf) ((struct uip_udp_hdr *)&buf[UIP_LLIPH_LEN])

#define UIP_IP_BUF(buf)          ((struct uip_ip_hdr *)&uip_buf(buf)[UIP_LLH_LEN])
#define UIP_UDP_BUF(buf)          ((struct uip_udp_hdr *)&uip_buf(buf)[UIP_LLIPH_LEN])
#define UIP_TCP_BUF(buf)          ((struct uip_tcp_hdr *)&uip_buf(buf)[UIP_LLIPH_LEN])
#define UIP_ICMP_BUF(buf)          ((struct uip_icmp_hdr *)&uip_buf(buf)[UIP_LLIPH_LEN])
/** @} */

#define sicslowpan_buf uip_buf
#define sicslowpan_len uip_len

/** \brief Maximum available size for frame headers,
           link layer security-related overhead, as well as
           6LoWPAN payload. */
#ifdef SICSLOWPAN_CONF_MAC_MAX_PAYLOAD
#define MAC_MAX_PAYLOAD SICSLOWPAN_CONF_MAC_MAX_PAYLOAD
#else /* SICSLOWPAN_CONF_MAC_MAX_PAYLOAD */
#define MAC_MAX_PAYLOAD (127 - 2)
#endif /* SICSLOWPAN_CONF_MAC_MAX_PAYLOAD */


/** \brief Some MAC layers need a minimum payload, which is
    configurable through the SICSLOWPAN_CONF_MIN_MAC_PAYLOAD
    option. */
#ifdef SICSLOWPAN_CONF_COMPRESSION_THRESHOLD
#define COMPRESSION_THRESHOLD SICSLOWPAN_CONF_COMPRESSION_THRESHOLD
#else
#define COMPRESSION_THRESHOLD 0
#endif

#if SICSLOWPAN_COMPRESSION == SICSLOWPAN_COMPRESSION_HC06
/** \name HC06 specific variables
 *  @{
 */

/** Addresses contexts for IPHC. */
#if SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS > 0
static struct sicslowpan_addr_context
addr_contexts[SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS];
#endif

/** pointer to an address context. */
static struct sicslowpan_addr_context *context;

/** pointer to the byte where to write next inline field. */
static uint8_t *hc06_ptr;

/* Uncompression of linklocal */
/*   0 -> 16 bytes from packet  */
/*   1 -> 2 bytes from prefix - bunch of zeroes and 8 from packet */
/*   2 -> 2 bytes from prefix - 0000::00ff:fe00:XXXX from packet */
/*   3 -> 2 bytes from prefix - infer 8 bytes from lladdr */
/*   NOTE: => the uncompress function does change 0xf to 0x10 */
/*   NOTE: 0x00 => no-autoconfig => unspecified */
const uint8_t unc_llconf[] = {0x0f,0x28,0x22,0x20};

/* Uncompression of ctx-based */
/*   0 -> 0 bits from packet [unspecified / reserved] */
/*   1 -> 8 bytes from prefix - bunch of zeroes and 8 from packet */
/*   2 -> 8 bytes from prefix - 0000::00ff:fe00:XXXX + 2 from packet */
/*   3 -> 8 bytes from prefix - infer 8 bytes from lladdr */
const uint8_t unc_ctxconf[] = {0x00,0x88,0x82,0x80};

/* Uncompression of ctx-based */
/*   0 -> 0 bits from packet  */
/*   1 -> 2 bytes from prefix - bunch of zeroes 5 from packet */
/*   2 -> 2 bytes from prefix - zeroes + 3 from packet */
/*   3 -> 2 bytes from prefix - infer 1 bytes from lladdr */
const uint8_t unc_mxconf[] = {0x0f, 0x25, 0x23, 0x21};

/* Link local prefix */
const uint8_t llprefix[] = {0xfe, 0x80};

/* TTL uncompression values */
static const uint8_t ttl_values[] = {0, 1, 64, 255};

/*--------------------------------------------------------------------*/
/** \name HC06 related functions
 * @{                                                                 */
/*--------------------------------------------------------------------*/
/** \brief find the context corresponding to prefix ipaddr */
static struct sicslowpan_addr_context*
addr_context_lookup_by_prefix(uip_ipaddr_t *ipaddr)
{
/* Remove code to avoid warnings and save flash if no context is used */
#if SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS > 0
  int i;
  for(i = 0; i < SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS; i++) {
    if((addr_contexts[i].used == 1) &&
       uip_ipaddr_prefixcmp(&addr_contexts[i].prefix, ipaddr, 64)) {
      return &addr_contexts[i];
    }
  }
#endif /* SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS > 0 */
  return NULL;
}
/*--------------------------------------------------------------------*/
/** \brief find the context with the given number */
static struct sicslowpan_addr_context*
addr_context_lookup_by_number(uint8_t number)
{
/* Remove code to avoid warnings and save flash if no context is used */
#if SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS > 0
  int i;
  for(i = 0; i < SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS; i++) {
    if((addr_contexts[i].used == 1) &&
       addr_contexts[i].number == number) {
      return &addr_contexts[i];
    }
  }
#endif /* SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS > 0 */
  return NULL;
}
/*--------------------------------------------------------------------*/
static uint8_t
compress_addr_64(uint8_t bitpos, uip_ipaddr_t *ipaddr, uip_lladdr_t *lladdr)
{
  if(uip_is_addr_mac_addr_based(ipaddr, lladdr)) {
    return 3 << bitpos; /* 0-bits */
  } else if(sicslowpan_is_iid_16_bit_compressable(ipaddr)) {
    /* compress IID to 16 bits xxxx::0000:00ff:fe00:XXXX */
    memcpy(hc06_ptr, &ipaddr->u16[7], 2);
    hc06_ptr += 2;
    return 2 << bitpos; /* 16-bits */
  } else {
    /* do not compress IID => xxxx::IID */
    memcpy(hc06_ptr, &ipaddr->u16[4], 8);
    hc06_ptr += 8;
    return 1 << bitpos; /* 64-bits */
  }
}

/*-------------------------------------------------------------------- */
/* Uncompress addresses based on a prefix and a postfix with zeroes in
 * between. If the postfix is zero in length it will use the link address
 * to configure the IP address (autoconf style).
 * pref_post_count takes a byte where the first nibble specify prefix count
 * and the second postfix count (NOTE: 15/0xf => 16 bytes copy).
 */
static void
uncompress_addr(uip_ipaddr_t *ipaddr, uint8_t const prefix[],
                uint8_t pref_post_count, uip_lladdr_t *lladdr)
{
  uint8_t prefcount = pref_post_count >> 4;
  uint8_t postcount = pref_post_count & 0x0f;
  /* full nibble 15 => 16 */
  prefcount = prefcount == 15 ? 16 : prefcount;
  postcount = postcount == 15 ? 16 : postcount;

  PRINTF("Uncompressing %d + %d => ", prefcount, postcount);

  if(prefcount > 0) {
    memcpy(ipaddr, prefix, prefcount);
  }
  if(prefcount + postcount < 16) {
    memset(&ipaddr->u8[prefcount], 0, 16 - (prefcount + postcount));
  }
  if(postcount > 0) {
    memcpy(&ipaddr->u8[16 - postcount], hc06_ptr, postcount);
    if(postcount == 2 && prefcount < 11) {
      /* 16 bits uncompression => 0000:00ff:fe00:XXXX */
      ipaddr->u8[11] = 0xff;
      ipaddr->u8[12] = 0xfe;
    }
    hc06_ptr += postcount;
  } else if (prefcount > 0) {
    /* no IID based configuration if no prefix and no data => unspec */
    uip_ds6_set_addr_iid(ipaddr, lladdr);
  }

  PRINT6ADDR(ipaddr);
  PRINTF("\n");
}

/*--------------------------------------------------------------------*/
/**
 * \brief Compress IP/UDP header
 *
 * This function is called by the 6lowpan code to create a compressed
 * 6lowpan packet in the packetbuf buffer from a full IPv6 packet in the
 * uip_buf buffer.
 *
 *
 * HC-06 (draft-ietf-6lowpan-hc, version 6)\n
 * http://tools.ietf.org/html/draft-ietf-6lowpan-hc-06
 *
 * \note We do not support ISA100_UDP header compression
 *
 * For LOWPAN_UDP compression, we either compress both ports or none.
 * General format with LOWPAN_UDP compression is
 * \verbatim
 *                      1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |0|1|1|TF |N|HLI|C|S|SAM|M|D|DAM| SCI   | DCI   | comp. IPv6 hdr|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | compressed IPv6 fields .....                                  |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | LOWPAN_UDP    | non compressed UDP fields ...                 |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | L4 data ...                                                   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * \endverbatim
 * \note The context number 00 is reserved for the link local prefix.
 * For unicast addresses, if we cannot compress the prefix, we neither
 * compress the IID.
 * \param link_destaddr L2 destination address, needed to compress IP
 * dest
 */
static int
compress_hdr_hc06(struct net_mbuf *mbuf, struct net_buf *buf, linkaddr_t *link_destaddr)
{
  uint8_t tmp, iphc0, iphc1;

  hc06_ptr = uip_packetbuf_ptr(mbuf) + 2;
  /*
   * As we copy some bit-length fields, in the IPHC encoding bytes,
   * we sometimes use |=
   * If the field is 0, and the current bit value in memory is 1,
   * this does not work. We therefore reset the IPHC encoding here
   */

  iphc0 = SICSLOWPAN_DISPATCH_IPHC;
  iphc1 = 0;
  PACKETBUF_IPHC_BUF(mbuf)[2] = 0; /* might not be used - but needs to be cleared */

  /*
   * Address handling needs to be made first since it might
   * cause an extra byte with [ SCI | DCI ]
   *
   */


  /* check if dest context exists (for allocating third byte) */
  /* TODO: fix this so that it remembers the looked up values for
     avoiding two lookups - or set the lookup values immediately */
  if(addr_context_lookup_by_prefix(&UIP_IP_BUF(buf)->destipaddr) != NULL ||
     addr_context_lookup_by_prefix(&UIP_IP_BUF(buf)->srcipaddr) != NULL) {
    /* set context flag and increase hc06_ptr */
    PRINTF("IPHC: compressing dest or src ipaddr - setting CID\n");
    iphc1 |= SICSLOWPAN_IPHC_CID;
    hc06_ptr++;
  }

  /*
   * Traffic class, flow label
   * If flow label is 0, compress it. If traffic class is 0, compress it
   * We have to process both in the same time as the offset of traffic class
   * depends on the presence of version and flow label
   */

  /* hc06 format of tc is ECN | DSCP , original is DSCP | ECN */
  tmp = (UIP_IP_BUF(buf)->vtc << 4) | (UIP_IP_BUF(buf)->tcflow >> 4);
  tmp = ((tmp & 0x03) << 6) | (tmp >> 2);

  if(((UIP_IP_BUF(buf)->tcflow & 0x0F) == 0) &&
     (UIP_IP_BUF(buf)->flow == 0)) {
    /* flow label can be compressed */
    iphc0 |= SICSLOWPAN_IPHC_FL_C;
    if(((UIP_IP_BUF(buf)->vtc & 0x0F) == 0) &&
       ((UIP_IP_BUF(buf)->tcflow & 0xF0) == 0)) {
      /* compress (elide) all */
      iphc0 |= SICSLOWPAN_IPHC_TC_C;
    } else {
      /* compress only the flow label */
     *hc06_ptr = tmp;
      hc06_ptr += 1;
    }
  } else {
    /* Flow label cannot be compressed */
    if(((UIP_IP_BUF(buf)->vtc & 0x0F) == 0) &&
       ((UIP_IP_BUF(buf)->tcflow & 0xF0) == 0)) {
      /* compress only traffic class */
      iphc0 |= SICSLOWPAN_IPHC_TC_C;
      *hc06_ptr = (tmp & 0xc0) |
        (UIP_IP_BUF(buf)->tcflow & 0x0F);
      memcpy(hc06_ptr + 1, &UIP_IP_BUF(buf)->flow, 2);
      hc06_ptr += 3;
    } else {
      /* compress nothing */
      memcpy(hc06_ptr, &UIP_IP_BUF(buf)->vtc, 4);
      /* but replace the top byte with the new ECN | DSCP format*/
      *hc06_ptr = tmp;
      hc06_ptr += 4;
   }
  }

  /* Note that the payload length is always compressed */

  /* Next header. We compress it if UDP */
#if UIP_CONF_UDP || UIP_CONF_ROUTER
  if(UIP_IP_BUF(buf)->proto == UIP_PROTO_UDP) {
    iphc0 |= SICSLOWPAN_IPHC_NH_C;
  }
#endif /*UIP_CONF_UDP*/
#ifdef SICSLOWPAN_NH_COMPRESSOR
  if(SICSLOWPAN_NH_COMPRESSOR.is_compressable(UIP_IP_BUF(buf)->proto)) {
    iphc0 |= SICSLOWPAN_IPHC_NH_C;
  }
#endif
  if ((iphc0 & SICSLOWPAN_IPHC_NH_C) == 0) {
    *hc06_ptr = UIP_IP_BUF(buf)->proto;
    hc06_ptr += 1;
  }

  /*
   * Hop limit
   * if 1: compress, encoding is 01
   * if 64: compress, encoding is 10
   * if 255: compress, encoding is 11
   * else do not compress
   */
  switch(UIP_IP_BUF(buf)->ttl) {
    case 1:
      iphc0 |= SICSLOWPAN_IPHC_TTL_1;
      break;
    case 64:
      iphc0 |= SICSLOWPAN_IPHC_TTL_64;
      break;
    case 255:
      iphc0 |= SICSLOWPAN_IPHC_TTL_255;
      break;
    default:
      *hc06_ptr = UIP_IP_BUF(buf)->ttl;
      hc06_ptr += 1;
      break;
  }

  /* source address - cannot be multicast */
  if(uip_is_addr_unspecified(&UIP_IP_BUF(buf)->srcipaddr)) {
    PRINTF("IPHC: compressing unspecified - setting SAC\n");
    iphc1 |= SICSLOWPAN_IPHC_SAC;
    iphc1 |= SICSLOWPAN_IPHC_SAM_00;
  } else if((context = addr_context_lookup_by_prefix(&UIP_IP_BUF(buf)->srcipaddr))
     != NULL) {
    /* elide the prefix - indicate by CID and set context + SAC */
    PRINTF("IPHC: compressing src with context - setting CID & SAC ctx: %d\n",
	   context->number);
    iphc1 |= SICSLOWPAN_IPHC_CID | SICSLOWPAN_IPHC_SAC;
    PACKETBUF_IPHC_BUF(mbuf)[2] |= context->number << 4;
    /* compession compare with this nodes address (source) */

    iphc1 |= compress_addr_64(SICSLOWPAN_IPHC_SAM_BIT,
                              &UIP_IP_BUF(buf)->srcipaddr, &uip_lladdr);
    /* No context found for this address */
  } else if(uip_is_addr_link_local(&UIP_IP_BUF(buf)->srcipaddr) &&
	    UIP_IP_BUF(buf)->destipaddr.u16[1] == 0 &&
	    UIP_IP_BUF(buf)->destipaddr.u16[2] == 0 &&
	    UIP_IP_BUF(buf)->destipaddr.u16[3] == 0) {
    iphc1 |= compress_addr_64(SICSLOWPAN_IPHC_SAM_BIT,
                              &UIP_IP_BUF(buf)->srcipaddr, &uip_lladdr);
  } else {
    /* send the full address => SAC = 0, SAM = 00 */
    iphc1 |= SICSLOWPAN_IPHC_SAM_00; /* 128-bits */
    memcpy(hc06_ptr, &UIP_IP_BUF(buf)->srcipaddr.u16[0], 16);
    hc06_ptr += 16;
  }

  /* dest address*/
  if(uip_is_addr_mcast(&UIP_IP_BUF(buf)->destipaddr)) {
    /* Address is multicast, try to compress */
    iphc1 |= SICSLOWPAN_IPHC_M;
    if(sicslowpan_is_mcast_addr_compressable8(&UIP_IP_BUF(buf)->destipaddr)) {
      iphc1 |= SICSLOWPAN_IPHC_DAM_11;
      /* use last byte */
      *hc06_ptr = UIP_IP_BUF(buf)->destipaddr.u8[15];
      hc06_ptr += 1;
    } else if(sicslowpan_is_mcast_addr_compressable32(&UIP_IP_BUF(buf)->destipaddr)) {
      iphc1 |= SICSLOWPAN_IPHC_DAM_10;
      /* second byte + the last three */
      *hc06_ptr = UIP_IP_BUF(buf)->destipaddr.u8[1];
      memcpy(hc06_ptr + 1, &UIP_IP_BUF(buf)->destipaddr.u8[13], 3);
      hc06_ptr += 4;
    } else if(sicslowpan_is_mcast_addr_compressable48(&UIP_IP_BUF(buf)->destipaddr)) {
      iphc1 |= SICSLOWPAN_IPHC_DAM_01;
      /* second byte + the last five */
      *hc06_ptr = UIP_IP_BUF(buf)->destipaddr.u8[1];
      memcpy(hc06_ptr + 1, &UIP_IP_BUF(buf)->destipaddr.u8[11], 5);
      hc06_ptr += 6;
    } else {
      iphc1 |= SICSLOWPAN_IPHC_DAM_00;
      /* full address */
      memcpy(hc06_ptr, &UIP_IP_BUF(buf)->destipaddr.u8[0], 16);
      hc06_ptr += 16;
    }
  } else {
    /* Address is unicast, try to compress */
    if((context = addr_context_lookup_by_prefix(&UIP_IP_BUF(buf)->destipaddr)) != NULL) {
      /* elide the prefix */
      iphc1 |= SICSLOWPAN_IPHC_DAC;
      PACKETBUF_IPHC_BUF(mbuf)[2] |= context->number;
      /* compession compare with link adress (destination) */

      iphc1 |= compress_addr_64(SICSLOWPAN_IPHC_DAM_BIT,
	       &UIP_IP_BUF(buf)->destipaddr, (uip_lladdr_t *)link_destaddr);
      /* No context found for this address */
    } else if(uip_is_addr_link_local(&UIP_IP_BUF(buf)->destipaddr) &&
	      UIP_IP_BUF(buf)->destipaddr.u16[1] == 0 &&
	      UIP_IP_BUF(buf)->destipaddr.u16[2] == 0 &&
	      UIP_IP_BUF(buf)->destipaddr.u16[3] == 0) {
      iphc1 |= compress_addr_64(SICSLOWPAN_IPHC_DAM_BIT,
               &UIP_IP_BUF(buf)->destipaddr, (uip_lladdr_t *)link_destaddr);
    } else {
      /* send the full address */
      iphc1 |= SICSLOWPAN_IPHC_DAM_00; /* 128-bits */
      memcpy(hc06_ptr, &UIP_IP_BUF(buf)->destipaddr.u16[0], 16);
      hc06_ptr += 16;
    }
  }

  uip_uncomp_hdr_len(mbuf) = UIP_IPH_LEN;

#if UIP_CONF_UDP || UIP_CONF_ROUTER
  /* UDP header compression */
  if(UIP_IP_BUF(buf)->proto == UIP_PROTO_UDP) {
    PRINTF("IPHC: Uncompressed UDP ports on send side: %x, %x\n",
	   UIP_HTONS(UIP_UDP_BUF(buf)->srcport), UIP_HTONS(UIP_UDP_BUF(buf)->destport));
    /* Mask out the last 4 bits can be used as a mask */
    if(((UIP_HTONS(UIP_UDP_BUF(buf)->srcport) & 0xfff0) == SICSLOWPAN_UDP_4_BIT_PORT_MIN) &&
       ((UIP_HTONS(UIP_UDP_BUF(buf)->destport) & 0xfff0) == SICSLOWPAN_UDP_4_BIT_PORT_MIN)) {
      /* we can compress 12 bits of both source and dest */
      *hc06_ptr = SICSLOWPAN_NHC_UDP_CS_P_11;
      PRINTF("IPHC: remove 12 b of both source & dest with prefix 0xFOB\n");
      *(hc06_ptr + 1) =
	(uint8_t)((UIP_HTONS(UIP_UDP_BUF(buf)->srcport) -
		SICSLOWPAN_UDP_4_BIT_PORT_MIN) << 4) +
	(uint8_t)((UIP_HTONS(UIP_UDP_BUF(buf)->destport) -
		SICSLOWPAN_UDP_4_BIT_PORT_MIN));
      hc06_ptr += 2;
    } else if((UIP_HTONS(UIP_UDP_BUF(buf)->destport) & 0xff00) == SICSLOWPAN_UDP_8_BIT_PORT_MIN) {
      /* we can compress 8 bits of dest, leave source. */
      *hc06_ptr = SICSLOWPAN_NHC_UDP_CS_P_01;
      PRINTF("IPHC: leave source, remove 8 bits of dest with prefix 0xF0\n");
      memcpy(hc06_ptr + 1, &UIP_UDP_BUF(buf)->srcport, 2);
      *(hc06_ptr + 3) =
	(uint8_t)((UIP_HTONS(UIP_UDP_BUF(buf)->destport) -
		SICSLOWPAN_UDP_8_BIT_PORT_MIN));
      hc06_ptr += 4;
    } else if((UIP_HTONS(UIP_UDP_BUF(buf)->srcport) & 0xff00) == SICSLOWPAN_UDP_8_BIT_PORT_MIN) {
      /* we can compress 8 bits of src, leave dest. Copy compressed port */
      *hc06_ptr = SICSLOWPAN_NHC_UDP_CS_P_10;
      PRINTF("IPHC: remove 8 bits of source with prefix 0xF0, leave dest. hch: %i\n", *hc06_ptr);
      *(hc06_ptr + 1) =
	(uint8_t)((UIP_HTONS(UIP_UDP_BUF(buf)->srcport) -
		SICSLOWPAN_UDP_8_BIT_PORT_MIN));
      memcpy(hc06_ptr + 2, &UIP_UDP_BUF(buf)->destport, 2);
      hc06_ptr += 4;
    } else {
      /* we cannot compress. Copy uncompressed ports, full checksum  */
      *hc06_ptr = SICSLOWPAN_NHC_UDP_CS_P_00;
      PRINTF("IPHC: cannot compress headers\n");
      memcpy(hc06_ptr + 1, &UIP_UDP_BUF(buf)->srcport, 4);
      hc06_ptr += 5;
    }
    /* always inline the checksum  */
    if(1) {
      memcpy(hc06_ptr, &UIP_UDP_BUF(buf)->udpchksum, 2);
      hc06_ptr += 2;
    }
    uip_uncomp_hdr_len(mbuf) += UIP_UDPH_LEN;
  }
#endif /*UIP_CONF_UDP*/

#ifdef SICSLOWPAN_NH_COMPRESSOR
  /* if nothing to compress just return zero  */
  hc06_ptr += SICSLOWPAN_NH_COMPRESSOR.compress(hc06_ptr, &uip_uncomp_hdr_len(mbuf));
#endif

  /* before the packetbuf_hdr_len operation */
  PACKETBUF_IPHC_BUF(mbuf)[0] = iphc0;
  PACKETBUF_IPHC_BUF(mbuf)[1] = iphc1;

  uip_packetbuf_hdr_len(mbuf) = hc06_ptr - uip_packetbuf_ptr(mbuf);

  return 1;
}

/*--------------------------------------------------------------------*/
/**
 * \brief Uncompress HC06 (i.e., IPHC and LOWPAN_UDP) headers and put
 * them in sicslowpan_buf
 *
 * This function is called by the input function when the dispatch is
 * HC06.
 * We %process the packet in the packetbuf buffer, uncompress the header
 * fields, and copy the result in the sicslowpan buffer.
 * At the end of the decompression, packetbuf_hdr_len and uncompressed_hdr_len
 * are set to the appropriate values
 *
 * \param ip_len Equal to 0 if the packet is not a fragment (IP length
 * is then inferred from the L2 length), non 0 if the packet is a 1st
 * fragment.
 */
static int
uncompress_hdr_hc06(struct net_mbuf *mbuf, struct net_buf *ibuf)
{
  uint8_t tmp, iphc0, iphc1;
  uint8_t buf[UIP_IPUDPH_LEN] = {}; /* Size of (IP + UDP)  header*/
  int ip_len;
  /* at least two byte will be used for the encoding */
  hc06_ptr = uip_packetbuf_ptr(mbuf) + uip_packetbuf_hdr_len(mbuf) + 2;

  iphc0 = PACKETBUF_IPHC_BUF(mbuf)[0];
  iphc1 = PACKETBUF_IPHC_BUF(mbuf)[1];

  /* another if the CID flag is set */
  if(iphc1 & SICSLOWPAN_IPHC_CID) {
    PRINTF("IPHC: CID flag set - increase header with one\n");
    hc06_ptr++;
  }

  /* Traffic class and flow label */
    if((iphc0 & SICSLOWPAN_IPHC_FL_C) == 0) {
      /* Flow label are carried inline */
      if((iphc0 & SICSLOWPAN_IPHC_TC_C) == 0) {
        /* Traffic class is carried inline */
        memcpy(&SICSLOWPAN_IP_BUF(buf)->tcflow, hc06_ptr + 1, 3);
        tmp = *hc06_ptr;
        hc06_ptr += 4;
        /* hc06 format of tc is ECN | DSCP , original is DSCP | ECN */
        /* set version, pick highest DSCP bits and set in vtc */
        SICSLOWPAN_IP_BUF(buf)->vtc = 0x60 | ((tmp >> 2) & 0x0f);
        /* ECN rolled down two steps + lowest DSCP bits at top two bits */
        SICSLOWPAN_IP_BUF(buf)->tcflow = ((tmp >> 2) & 0x30) | (tmp << 6) |
	(SICSLOWPAN_IP_BUF(buf)->tcflow & 0x0f);
      } else {
        /* Traffic class is compressed (set version and no TC)*/
        SICSLOWPAN_IP_BUF(buf)->vtc = 0x60;
        /* highest flow label bits + ECN bits */
        SICSLOWPAN_IP_BUF(buf)->tcflow = (*hc06_ptr & 0x0F) |
	((*hc06_ptr >> 2) & 0x30);
        memcpy(&SICSLOWPAN_IP_BUF(buf)->flow, hc06_ptr + 1, 2);
        hc06_ptr += 3;
      }
    } else {
      /* Version is always 6! */
      /* Version and flow label are compressed */
      if((iphc0 & SICSLOWPAN_IPHC_TC_C) == 0) {
        /* Traffic class is inline */
          SICSLOWPAN_IP_BUF(buf)->vtc = 0x60 | ((*hc06_ptr >> 2) & 0x0f);
          SICSLOWPAN_IP_BUF(buf)->tcflow = ((*hc06_ptr << 6) & 0xC0) | ((*hc06_ptr >> 2) & 0x30);
          SICSLOWPAN_IP_BUF(buf)->flow = 0;
          hc06_ptr += 1;
      } else {
        /* Traffic class is compressed */
        SICSLOWPAN_IP_BUF(buf)->vtc = 0x60;
        SICSLOWPAN_IP_BUF(buf)->tcflow = 0;
        SICSLOWPAN_IP_BUF(buf)->flow = 0;
      }
    }

  /* Next Header */
  if((iphc0 & SICSLOWPAN_IPHC_NH_C) == 0) {
    /* Next header is carried inline */
    SICSLOWPAN_IP_BUF(buf)->proto = *hc06_ptr;
    PRINTF("IPHC: next header inline: %d\n", SICSLOWPAN_IP_BUF(buf)->proto);
    hc06_ptr += 1;
  }

  /* Hop limit */
  if((iphc0 & 0x03) != SICSLOWPAN_IPHC_TTL_I) {
    SICSLOWPAN_IP_BUF(buf)->ttl = ttl_values[iphc0 & 0x03];
  } else {
    SICSLOWPAN_IP_BUF(buf)->ttl = *hc06_ptr;
    hc06_ptr += 1;
  }

  /* put the source address compression mode SAM in the tmp var */
  tmp = ((iphc1 & SICSLOWPAN_IPHC_SAM_11) >> SICSLOWPAN_IPHC_SAM_BIT) & 0x03;

  /* context based compression */
  if(iphc1 & SICSLOWPAN_IPHC_SAC) {
    uint8_t sci = (iphc1 & SICSLOWPAN_IPHC_CID) ?
      PACKETBUF_IPHC_BUF(mbuf)[2] >> 4 : 0;

    /* Source address - check context != NULL only if SAM bits are != 0*/
    if (tmp != 0) {
      context = addr_context_lookup_by_number(sci);
      if(context == NULL) {
        PRINTF("sicslowpan uncompress_hdr: error context not found\n");
        return 0;
      }
    }
    /* if tmp == 0 we do not have a context and therefore no prefix */
    uncompress_addr(&SICSLOWPAN_IP_BUF(buf)->srcipaddr,
                    tmp != 0 ? context->prefix : NULL, unc_ctxconf[tmp],
                    (uip_lladdr_t *)&ibuf->src);
  } else {
    /* no compression and link local */
    uncompress_addr(&SICSLOWPAN_IP_BUF(buf)->srcipaddr, llprefix, unc_llconf[tmp],
                    (uip_lladdr_t *)&ibuf->src);
  }

  /* Destination address */
  /* put the destination address compression mode into tmp */
  tmp = ((iphc1 & SICSLOWPAN_IPHC_DAM_11) >> SICSLOWPAN_IPHC_DAM_BIT) & 0x03;

  /* multicast compression */
  if(iphc1 & SICSLOWPAN_IPHC_M) {
    /* context based multicast compression */
    if(iphc1 & SICSLOWPAN_IPHC_DAC) {
      /* TODO: implement this */
    } else {
      /* non-context based multicast compression - */
      /* DAM_00: 128 bits  */
      /* DAM_01:  48 bits FFXX::00XX:XXXX:XXXX */
      /* DAM_10:  32 bits FFXX::00XX:XXXX */
      /* DAM_11:   8 bits FF02::00XX */
      uint8_t prefix[] = {0xff, 0x02};
      if(tmp > 0 && tmp < 3) {
        prefix[1] = *hc06_ptr;
        hc06_ptr++;
      }

      uncompress_addr(&SICSLOWPAN_IP_BUF(buf)->destipaddr, prefix,
                      unc_mxconf[tmp], NULL);
    }
  } else {
    /* no multicast */
    /* Context based */
    if(iphc1 & SICSLOWPAN_IPHC_DAC) {
      uint8_t dci = (iphc1 & SICSLOWPAN_IPHC_CID) ?
	PACKETBUF_IPHC_BUF(mbuf)[2] & 0x0f : 0;
      context = addr_context_lookup_by_number(dci);

      /* all valid cases below need the context! */
      if(context == NULL) {
	PRINTF("sicslowpan uncompress_hdr: error context not found\n");
	return 0;
      }
      uncompress_addr(&SICSLOWPAN_IP_BUF(buf)->destipaddr, context->prefix,
                      unc_ctxconf[tmp], (uip_lladdr_t *)&ibuf->dest);
    } else {
      /* not context based => link local M = 0, DAC = 0 - same as SAC */
      uncompress_addr(&SICSLOWPAN_IP_BUF(buf)->destipaddr, llprefix,
                      unc_llconf[tmp], (uip_lladdr_t *)&ibuf->dest);
    }
  }
  uip_uncomp_hdr_len(mbuf) += UIP_IPH_LEN;

  /* Next header processing - continued */
  if((iphc0 & SICSLOWPAN_IPHC_NH_C)) {
    /* The next header is compressed, NHC is following */
    if((*hc06_ptr & SICSLOWPAN_NHC_UDP_MASK) == SICSLOWPAN_NHC_UDP_ID) {
      uint8_t checksum_compressed;
      SICSLOWPAN_IP_BUF(buf)->proto = UIP_PROTO_UDP;
      checksum_compressed = *hc06_ptr & SICSLOWPAN_NHC_UDP_CHECKSUMC;
      PRINTF("IPHC: Incoming header value: %x\n", *hc06_ptr);
      switch(*hc06_ptr & SICSLOWPAN_NHC_UDP_CS_P_11) {
      case SICSLOWPAN_NHC_UDP_CS_P_00:
	/* 1 byte for NHC, 4 byte for ports, 2 bytes chksum */
	memcpy(&SICSLOWPAN_UDP_BUF(buf)->srcport, hc06_ptr + 1, 2);
	memcpy(&SICSLOWPAN_UDP_BUF(buf)->destport, hc06_ptr + 3, 2);
	PRINTF("IPHC: Uncompressed UDP ports (ptr+5): %x, %x\n",
	       UIP_HTONS(SICSLOWPAN_UDP_BUF(buf)->srcport), UIP_HTONS(SICSLOWPAN_UDP_BUF(buf)->destport));
	hc06_ptr += 5;
	break;

      case SICSLOWPAN_NHC_UDP_CS_P_01:
        /* 1 byte for NHC + source 16bit inline, dest = 0xF0 + 8 bit inline */
	PRINTF("IPHC: Decompressing destination\n");
	memcpy(&SICSLOWPAN_UDP_BUF(buf)->srcport, hc06_ptr + 1, 2);
	SICSLOWPAN_UDP_BUF(buf)->destport = UIP_HTONS(SICSLOWPAN_UDP_8_BIT_PORT_MIN + (*(hc06_ptr + 3)));
	PRINTF("IPHC: Uncompressed UDP ports (ptr+4): %x, %x\n",
	       UIP_HTONS(SICSLOWPAN_UDP_BUF(buf)->srcport), UIP_HTONS(SICSLOWPAN_UDP_BUF(buf)->destport));
	hc06_ptr += 4;
	break;

      case SICSLOWPAN_NHC_UDP_CS_P_10:
        /* 1 byte for NHC + source = 0xF0 + 8bit inline, dest = 16 bit inline*/
	PRINTF("IPHC: Decompressing source\n");
	SICSLOWPAN_UDP_BUF(buf)->srcport = UIP_HTONS(SICSLOWPAN_UDP_8_BIT_PORT_MIN +
					    (*(hc06_ptr + 1)));
	memcpy(&SICSLOWPAN_UDP_BUF(buf)->destport, hc06_ptr + 2, 2);
	PRINTF("IPHC: Uncompressed UDP ports (ptr+4): %x, %x\n",
	       UIP_HTONS(SICSLOWPAN_UDP_BUF(buf)->srcport), UIP_HTONS(SICSLOWPAN_UDP_BUF(buf)->destport));
	hc06_ptr += 4;
	break;

      case SICSLOWPAN_NHC_UDP_CS_P_11:
	/* 1 byte for NHC, 1 byte for ports */
	SICSLOWPAN_UDP_BUF(buf)->srcport = UIP_HTONS(SICSLOWPAN_UDP_4_BIT_PORT_MIN +
					    (*(hc06_ptr + 1) >> 4));
	SICSLOWPAN_UDP_BUF(buf)->destport = UIP_HTONS(SICSLOWPAN_UDP_4_BIT_PORT_MIN +
					     ((*(hc06_ptr + 1)) & 0x0F));
	PRINTF("IPHC: Uncompressed UDP ports (ptr+2): %x, %x\n",
	       UIP_HTONS(SICSLOWPAN_UDP_BUF(buf)->srcport), UIP_HTONS(SICSLOWPAN_UDP_BUF(buf)->destport));
	hc06_ptr += 2;
	break;

      default:
	PRINTF("sicslowpan uncompress_hdr: error unsupported UDP compression\n");
	return 0;
      }
      if(!checksum_compressed) { /* has_checksum, default  */
	memcpy(&SICSLOWPAN_UDP_BUF(buf)->udpchksum, hc06_ptr, 2);
	hc06_ptr += 2;
	PRINTF("IPHC: sicslowpan uncompress_hdr: checksum included\n");
      } else {
	PRINTF("IPHC: sicslowpan uncompress_hdr: checksum *NOT* included\n");
      }
      uip_uncomp_hdr_len(mbuf) += UIP_UDPH_LEN;
    }
#ifdef SICSLOWPAN_NH_COMPRESSOR
    else {
      hc06_ptr += SICSLOWPAN_NH_COMPRESSOR.uncompress(hc06_ptr, sicslowpan_buf(buf), &uip_uncomp_hdr_len(buf));
    }
#endif
  }

  uip_packetbuf_hdr_len(mbuf) = hc06_ptr - uip_packetbuf_ptr(mbuf);

  ip_len = uip_len(ibuf) + (uip_uncomp_hdr_len(mbuf) -
                                 uip_packetbuf_hdr_len(mbuf)) - UIP_IPH_LEN;
    /* This is not a fragmented packet */
    SICSLOWPAN_IP_BUF(buf)->len[0] = ip_len >> 8;
    SICSLOWPAN_IP_BUF(buf)->len[1] = ip_len & 0x00FF;

  /* length field in UDP header */
  if(SICSLOWPAN_IP_BUF(buf)->proto == UIP_PROTO_UDP) {
    memcpy(&SICSLOWPAN_UDP_BUF(buf)->udplen, &SICSLOWPAN_IP_BUF(buf)->len[0], 2);
  }

  packetbuf_copyfrom(mbuf, buf, sizeof(buf));

  return 1;
}
/** @} */
#endif /* SICSLOWPAN_COMPRESSION == SICSLOWPAN_COMPRESSION_HC06 */


#if SICSLOWPAN_COMPRESSION == SICSLOWPAN_COMPRESSION_HC1
/*--------------------------------------------------------------------*/
/** \name HC1 compression and uncompression functions
 *  @{                                                                */
/*--------------------------------------------------------------------*/
/**
 * \brief Compress IP/UDP header using HC1 and HC_UDP
 *
 * This function is called by the 6lowpan code to create a compressed
 * 6lowpan packet in the packetbuf buffer from a full IPv6 packet in the
 * uip_buf buffer.
 *
 *
 * If we can compress everything, we use HC1 dispatch, if not we use
 * IPv6 dispatch.\n
 * We can compress everything if:
 *   - IP version is
 *   - Flow label and traffic class are 0
 *   - Both src and dest ip addresses are link local
 *   - Both src and dest interface ID are recoverable from lower layer
 *     header
 *   - Next header is either ICMP, UDP or TCP
 * Moreover, if next header is UDP, we try to compress it using HC_UDP.
 * This is feasible is both ports are between F0B0 and F0B0 + 15\n\n
 *
 * Resulting header structure:
 * - For ICMP, TCP, non compressed UDP\n
 *   HC1 encoding = 11111010 (UDP) 11111110 (TCP) 11111100 (ICMP)\n
 * \verbatim
 *                      1                   2                   3
 * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | LoWPAN HC1 Dsp | HC1 encoding  | IPv6 Hop limit| L4 hdr + data|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | ...
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * \endverbatim
 *
 * - For compressed UDP
 *   HC1 encoding = 11111011, HC_UDP encoding = 11100000\n
 * \verbatim
 *                      1                   2                   3
 * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | LoWPAN HC1 Dsp| HC1 encoding  |  HC_UDP encod.| IPv6 Hop limit|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | src p.| dst p.| UDP checksum                  | L4 data...
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * \endverbatim
 *
 * \param link_destaddr L2 destination address, needed to compress the
 * IP destination field
 */
static int
compress_hdr_hc1(struct net_mbuf *mbuf, struct net_buf *buf, linkaddr_t *link_destaddr)
{
  /*
   * Check if all the assumptions for full compression
   * are valid :
   */
  if(UIP_IP_BUF(buf)->vtc != 0x60 ||
     UIP_IP_BUF(buf)->tcflow != 0 ||
     UIP_IP_BUF(buf)->flow != 0 ||
     !uip_is_addr_link_local(&UIP_IP_BUF(buf)->srcipaddr) ||
     !uip_is_addr_mac_addr_based(&UIP_IP_BUF(buf)->srcipaddr, &uip_lladdr) ||
     !uip_is_addr_link_local(&UIP_IP_BUF(buf)->destipaddr) ||
     !uip_is_addr_mac_addr_based(&UIP_IP_BUF(buf)->destipaddr,
                                 (uip_lladdr_t *)link_destaddr) ||
     (UIP_IP_BUF(buf)->proto != UIP_PROTO_ICMP6 &&
      UIP_IP_BUF(buf)->proto != UIP_PROTO_UDP &&
      UIP_IP_BUF(buf)->proto != UIP_PROTO_TCP))
  {
     return 0;
  } else {
    /*
     * HC1 DISPATCH
     * maximum compresssion:
     * All fields in the IP header but Hop Limit are elided
     * If next header is UDP, we compress UDP header using HC2
     */
    PACKETBUF_HC1_PTR(mbuf)[PACKETBUF_HC1_DISPATCH] = SICSLOWPAN_DISPATCH_HC1;
    uip_uncomp_hdr_len(mbuf) += UIP_IPH_LEN;
    switch(UIP_IP_BUF(buf)->proto) {
      case UIP_PROTO_ICMP6:
        /* HC1 encoding and ttl */
        PACKETBUF_HC1_PTR(mbuf)[PACKETBUF_HC1_ENCODING] = 0xFC;
        PACKETBUF_HC1_PTR(mbuf)[PACKETBUF_HC1_TTL] = UIP_IP_BUF(buf)->ttl;
        uip_packetbuf_hdr_len(mbuf) += SICSLOWPAN_HC1_HDR_LEN;
        break;
#if UIP_CONF_TCP
      case UIP_PROTO_TCP:
        /* HC1 encoding and ttl */
        PACKETBUF_HC1_PTR[PACKETBUF_HC1_ENCODING] = 0xFE;
        PACKETBUF_HC1_PTR[PACKETBUF_HC1_TTL] = UIP_IP_BUF(buf)->ttl;
        uip_packetbuf_hdr_len(mbuf) += SICSLOWPAN_HC1_HDR_LEN;
        break;
#endif /* UIP_CONF_TCP */
#if UIP_CONF_UDP
      case UIP_PROTO_UDP:
        /*
         * try to compress UDP header (we do only full compression).
         * This is feasible if both src and dest ports are between
         * SICSLOWPAN_UDP_PORT_MIN and SICSLOWPAN_UDP_PORT_MIN + 15
         */
        PRINTF("local/remote port %x/%x\n",UIP_UDP_BUF(buf)->srcport,UIP_UDP_BUF(buf)->destport);
        if(UIP_HTONS(UIP_UDP_BUF(buf)->srcport)  >= SICSLOWPAN_UDP_PORT_MIN &&
           UIP_HTONS(UIP_UDP_BUF(buf)->srcport)  <  SICSLOWPAN_UDP_PORT_MAX &&
           UIP_HTONS(UIP_UDP_BUF(buf)->destport) >= SICSLOWPAN_UDP_PORT_MIN &&
           UIP_HTONS(UIP_UDP_BUF(buf)->destport) <  SICSLOWPAN_UDP_PORT_MAX) {
          /* HC1 encoding */
          PACKETBUF_HC1_HC_UDP_PTR(mbuf)[PACKETBUF_HC1_HC_UDP_HC1_ENCODING] = 0xFB;

          /* HC_UDP encoding, ttl, src and dest ports, checksum */
          PACKETBUF_HC1_HC_UDP_PTR(mbuf)[PACKETBUF_HC1_HC_UDP_UDP_ENCODING] = 0xE0;
          PACKETBUF_HC1_HC_UDP_PTR(mbuf)[PACKETBUF_HC1_HC_UDP_TTL] = UIP_IP_BUF(buf)->ttl;

          PACKETBUF_HC1_HC_UDP_PTR(mbuf)[PACKETBUF_HC1_HC_UDP_PORTS] =
               (uint8_t)((UIP_HTONS(UIP_UDP_BUF(buf)->srcport) -
                       SICSLOWPAN_UDP_PORT_MIN) << 4) +
               (uint8_t)((UIP_HTONS(UIP_UDP_BUF(buf)->destport) - SICSLOWPAN_UDP_PORT_MIN));
          memcpy(&PACKETBUF_HC1_HC_UDP_PTR(mbuf)[PACKETBUF_HC1_HC_UDP_CHKSUM], &UIP_UDP_BUF(buf)->udpchksum, 2);
          uip_packetbuf_hdr_len(mbuf) += SICSLOWPAN_HC1_HC_UDP_HDR_LEN;
          uip_uncomp_hdr_len(mbuf) += UIP_UDPH_LEN;
        } else {
          /* HC1 encoding and ttl */
          PACKETBUF_HC1_PTR(mbuf)[PACKETBUF_HC1_ENCODING] = 0xFA;
          PACKETBUF_HC1_PTR(mbuf)[PACKETBUF_HC1_TTL] = UIP_IP_BUF(buf)->ttl;
          uip_packetbuf_hdr_len(mbuf) += SICSLOWPAN_HC1_HDR_LEN;
        }
        break;
#endif /*UIP_CONF_UDP*/
    }
  }
  return 1;
}

/*--------------------------------------------------------------------*/
/**
 * \brief Uncompress HC1 (and HC_UDP) headers and put them in
 * sicslowpan_buf
 *
 * This function is called by the input function when the dispatch is
 * HC1.
 * We %process the packet in the packetbuf buffer, uncompress the header
 * fields, and copy the result in the sicslowpan buffer.
 * At the end of the decompression, packetbuf_hdr_len and uncompressed_hdr_len
 * are set to the appropriate values
 *
 * \param ip_len Equal to 0 if the packet is not a fragment (IP length
 * is then inferred from the L2 length), non 0 if the packet is a 1st
 * fragment.
 */
static int
uncompress_hdr_hc1(struct net_mbuf *mbuf, struct net_buf *ibuf)
{
  uint8_t buf[UIP_IPUDPH_LEN] = {}; /* Size of (IP + UDP)  header*/
  int ip_len;

  /* version, traffic class, flow label */
  SICSLOWPAN_IP_BUF(buf)->vtc = 0x60;
  SICSLOWPAN_IP_BUF(buf)->tcflow = 0;
  SICSLOWPAN_IP_BUF(buf)->flow = 0;

  /* src and dest ip addresses */
  uip_ip6addr(&SICSLOWPAN_IP_BUF(buf)->srcipaddr, 0xfe80, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&SICSLOWPAN_IP_BUF(buf)->srcipaddr, (uip_lladdr_t *)&ibuf->src);
  uip_ip6addr(&SICSLOWPAN_IP_BUF(buf)->destipaddr, 0xfe80, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&SICSLOWPAN_IP_BUF(buf)->destipaddr, (uip_lladdr_t *)&ibuf->dest);

  uip_uncomp_hdr_len(mbuf) += UIP_IPH_LEN;

  /* Next header field */
  switch(PACKETBUF_HC1_PTR(mbuf)[PACKETBUF_HC1_ENCODING] & 0x06) {
    case SICSLOWPAN_HC1_NH_ICMP6:
      SICSLOWPAN_IP_BUF(buf)->proto = UIP_PROTO_ICMP6;
      SICSLOWPAN_IP_BUF(buf)->ttl = PACKETBUF_HC1_PTR(mbuf)[PACKETBUF_HC1_TTL];
      uip_packetbuf_hdr_len(mbuf) += SICSLOWPAN_HC1_HDR_LEN;
      break;
#if UIP_CONF_TCP
    case SICSLOWPAN_HC1_NH_TCP:
      SICSLOWPAN_IP_BUF(buf)->proto = UIP_PROTO_TCP;
      SICSLOWPAN_IP_BUF(buf)->ttl = PACKETBUF_HC1_PTR(mbuf)[PACKETBUF_HC1_TTL];
      uip_packetbuf_hdr_len(mbuf) += SICSLOWPAN_HC1_HDR_LEN;
      break;
#endif/* UIP_CONF_TCP */
#if UIP_CONF_UDP
    case SICSLOWPAN_HC1_NH_UDP:
      SICSLOWPAN_IP_BUF(buf)->proto = UIP_PROTO_UDP;
      if(PACKETBUF_HC1_HC_UDP_PTR(mbuf)[PACKETBUF_HC1_HC_UDP_HC1_ENCODING] & 0x01) {
        /* UDP header is compressed with HC_UDP */
        if(PACKETBUF_HC1_HC_UDP_PTR(mbuf)[PACKETBUF_HC1_HC_UDP_UDP_ENCODING] !=
           SICSLOWPAN_HC_UDP_ALL_C) {
          PRINTF("sicslowpan (uncompress_hdr), packet not supported");
          return 0;
        }
        /* IP TTL */
        SICSLOWPAN_IP_BUF(buf)->ttl = PACKETBUF_HC1_HC_UDP_PTR(mbuf)[PACKETBUF_HC1_HC_UDP_TTL];
        /* UDP ports, len, checksum */
        SICSLOWPAN_UDP_BUF(buf)->srcport =
          UIP_HTONS(SICSLOWPAN_UDP_PORT_MIN +
                (PACKETBUF_HC1_HC_UDP_PTR(mbuf)[PACKETBUF_HC1_HC_UDP_PORTS] >> 4));
        SICSLOWPAN_UDP_BUF(buf)->destport =
          UIP_HTONS(SICSLOWPAN_UDP_PORT_MIN +
                (PACKETBUF_HC1_HC_UDP_PTR(mbuf)[PACKETBUF_HC1_HC_UDP_PORTS] & 0x0F));
        memcpy(&SICSLOWPAN_UDP_BUF(buf)->udpchksum, &PACKETBUF_HC1_HC_UDP_PTR(mbuf)[PACKETBUF_HC1_HC_UDP_CHKSUM], 2);
        uip_uncomp_hdr_len(mbuf) += UIP_UDPH_LEN;
        uip_packetbuf_hdr_len(mbuf) += SICSLOWPAN_HC1_HC_UDP_HDR_LEN;
      } else {
        uip_packetbuf_hdr_len(mbuf) += SICSLOWPAN_HC1_HDR_LEN;
      }
      break;
#endif/* UIP_CONF_UDP */
    default:
      /* this shouldn't happen, drop */
      return 0;
  }

  /* IP length field. */
   ip_len = uip_len(ibuf) + (uip_uncomp_hdr_len(mbuf) -
                               uip_packetbuf_hdr_len(mbuf)) - UIP_IPH_LEN;
  SICSLOWPAN_IP_BUF(buf)->len[0] = ip_len >> 8;
  SICSLOWPAN_IP_BUF(buf)->len[1] = ip_len & 0x00FF;

  /* length field in UDP header */
  if(SICSLOWPAN_IP_BUF(buf)->proto == UIP_PROTO_UDP) {
    memcpy(&SICSLOWPAN_UDP_BUF(buf)->udplen, &SICSLOWPAN_IP_BUF(buf)->len[0], 2);
  }

  packetbuf_copyfrom(mbuf, buf, sizeof(buf));
  return 1;
}
/** @} */
#endif /* SICSLOWPAN_COMPRESSION == SICSLOWPAN_COMPRESSION_HC1 */

/*--------------------------------------------------------------------*/
/** \name IPv6 dispatch "compression" function
 * @{                                                                 */
/*--------------------------------------------------------------------*/
/* \brief Packets "Compression" when only IPv6 dispatch is used
 *
 * There is no compression in this case, all fields are sent
 * inline. We just add the IPv6 dispatch byte before the packet.
 * \verbatim
 * 0               1                   2                   3
 * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | IPv6 Dsp      | IPv6 header and payload ...
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * \endverbatim
 */
static int
compress_hdr_ipv6(struct net_buf *buf)
{
  memmove(uip_buf(buf) + SICSLOWPAN_IPV6_HDR_LEN, uip_buf(buf), uip_len(buf));
  *uip_buf(buf) = SICSLOWPAN_DISPATCH_IPV6;
  uip_len(buf)++;
  return 1;
}
/** @} */

/* Just remove the IPv6 Dispatch */
static int
uncompress_hdr_ipv6(struct net_buf *buf)
{
  uip_len(buf)--;
  memmove(uip_buf(buf), uip_buf(buf) + SICSLOWPAN_IPV6_HDR_LEN, uip_len(buf));
  return 1;
}

/*--------------------------------------------------------------------*/
/** \name Input/output functions common to all compression schemes
 * @{                                                                 */
/*--------------------------------------------------------------------*/

static int compress(struct net_buf *buf)
{
  uint8_t hdr_diff;
  struct net_mbuf *mbuf;
  int ret;

  if(uip_len(buf) >= COMPRESSION_THRESHOLD) {
#if SICSLOWPAN_COMPRESSION == SICSLOWPAN_COMPRESSION_IPV6
    return compress_hdr_ipv6(buf);
#endif /* SICSLOWPAN_COMPRESSION == SICSLOWPAN_COMPRESSION_IPV6 */
  } else {
    return compress_hdr_ipv6(buf);
  }

  mbuf = net_mbuf_get_reserve(0);
  if (!mbuf) {
     return 0;
  }

  /* init */
  uip_uncomp_hdr_len(mbuf) = 0;
  uip_packetbuf_hdr_len(mbuf) = 0;
  /* reset packetbuf buffer */
  packetbuf_clear(mbuf);
  uip_packetbuf_ptr(mbuf) = packetbuf_dataptr(mbuf);

  /* Compress the headers */
#if SICSLOWPAN_COMPRESSION == SICSLOWPAN_COMPRESSION_HC1
   ret = compress_hdr_hc1(mbuf, buf, &buf->dest);
#endif /* SICSLOWPAN_COMPRESSION == SICSLOWPAN_COMPRESSION_HC1 */
#if SICSLOWPAN_COMPRESSION == SICSLOWPAN_COMPRESSION_HC06
   ret = compress_hdr_hc06(mbuf, buf, &buf->dest);
#endif /* SICSLOWPAN_COMPRESSION == SICSLOWPAN_COMPRESSION_HC06 */

  /* if HC1 or HC06 compression fails then send uncompressed ipv6 packet */
  if (!ret) {
     net_mbuf_put(mbuf);
     PRINTF("sending uncompressed IPv6 packet\n");
     return compress_hdr_ipv6(buf);
     return 0;
  }

  PRINTF("compress: compressed hdr len %d, uncompressed hdr len %d\n",
                     uip_packetbuf_hdr_len(mbuf), uip_uncomp_hdr_len(mbuf));
  hdr_diff = uip_uncomp_hdr_len(mbuf) - uip_packetbuf_hdr_len(mbuf);
  memcpy(uip_buf(buf), uip_packetbuf_ptr(mbuf), uip_packetbuf_hdr_len(mbuf));
  memmove(uip_buf(buf) + uip_packetbuf_hdr_len(mbuf),
            uip_buf(buf) + uip_uncomp_hdr_len(mbuf), uip_len(buf) - uip_uncomp_hdr_len(mbuf));
  uip_len(buf) -= hdr_diff;
  packetbuf_clear(mbuf);
  net_mbuf_put(mbuf);
  return 1;
}

static int uncompress(struct net_buf *buf)
{
  struct net_mbuf *mbuf;

  if (*uip_buf(buf) == SICSLOWPAN_DISPATCH_IPV6) {
        return uncompress_hdr_ipv6(buf);
  }

  mbuf = net_mbuf_get_reserve(0);
  if (!mbuf) {
     return 0;
  }

  /* init */
  uip_uncomp_hdr_len(mbuf) = 0;
  uip_packetbuf_hdr_len(mbuf) = 0;
  packetbuf_copyfrom(mbuf, uip_buf(buf), UIP_IPUDPH_LEN); /* Size of (IP + UDP)  header*/
  uip_packetbuf_ptr(mbuf) = packetbuf_dataptr(mbuf);

#if SICSLOWPAN_COMPRESSION == SICSLOWPAN_COMPRESSION_HC06
  if((PACKETBUF_HC1_PTR(mbuf)[PACKETBUF_HC1_DISPATCH] & 0xe0) == SICSLOWPAN_DISPATCH_IPHC) {
    PRINTFO("uncompress: IPHC\n");
    if(!uncompress_hdr_hc06(mbuf, buf)) {
       goto fail;
    }
  } else
#endif /* SICSLOWPAN_COMPRESSION == SICSLOWPAN_COMPRESSION_HC06 */
    switch(PACKETBUF_HC1_PTR(mbuf)[PACKETBUF_HC1_DISPATCH]) {
#if SICSLOWPAN_COMPRESSION == SICSLOWPAN_COMPRESSION_HC1
    case SICSLOWPAN_DISPATCH_HC1:
      PRINTFO("uncompress: HC1\n");
      if(!uncompress_hdr_hc1(mbuf, buf)) {
          goto fail;
      }
      break;
#endif /* SICSLOWPAN_COMPRESSION == SICSLOWPAN_COMPRESSION_HC1 */
    default:
      /* unknown header */
      PRINTFO("uncompress: unknown dispatch: %x--%x\n",
             PACKETBUF_HC1_PTR(mbuf)[PACKETBUF_HC1_DISPATCH], *uip_buf(buf));
      goto fail;
  }

#if SICSLOWPAN_COMPRESSION == SICSLOWPAN_COMPRESSION_HC06 || SICSLOWPAN_COMPRESSION == SICSLOWPAN_COMPRESSION_HC1
  memmove(uip_buf(buf) + uip_uncomp_hdr_len(mbuf),
                uip_buf(buf) + uip_packetbuf_hdr_len(mbuf),
                uip_len(buf) - uip_packetbuf_hdr_len(mbuf));
  memcpy(uip_buf(buf), uip_packetbuf_ptr(mbuf), uip_uncomp_hdr_len(mbuf));
  uip_len(buf) += (uip_uncomp_hdr_len(mbuf) - uip_packetbuf_hdr_len(mbuf));
#endif

  net_mbuf_put(mbuf);
  return 1;

fail:
  net_mbuf_put(mbuf);
  return 0;
}

static void init(void)
{
#if SICSLOWPAN_COMPRESSION == SICSLOWPAN_COMPRESSION_HC06
/* Preinitialize any address contexts for better header compression
 * (Saves up to 13 bytes per 6lowpan packet)
 * The platform contiki-conf.h file can override this using e.g.
 * #define SICSLOWPAN_CONF_ADDR_CONTEXT_0 {addr_contexts[0].prefix[0]=0xbb;addr_contexts[0].prefix[1]=0xbb;}
 */
#if SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS > 0
  addr_contexts[0].used   = 1;
  addr_contexts[0].number = 0;
#ifdef SICSLOWPAN_CONF_ADDR_CONTEXT_0
        SICSLOWPAN_CONF_ADDR_CONTEXT_0;
#else
  addr_contexts[0].prefix[0] = 0xaa;
  addr_contexts[0].prefix[1] = 0xaa;
#endif
#endif /* SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS > 0 */

#if SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS > 1
  {
    int i;
    for(i = 1; i < SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS; i++) {
#ifdef SICSLOWPAN_CONF_ADDR_CONTEXT_1
          if (i==1) {
            addr_contexts[1].used   = 1;
                addr_contexts[1].number = 1;
                SICSLOWPAN_CONF_ADDR_CONTEXT_1;
#ifdef SICSLOWPAN_CONF_ADDR_CONTEXT_2
      } else if (i==2) {
                addr_contexts[2].used   = 1;
                addr_contexts[2].number = 2;
                SICSLOWPAN_CONF_ADDR_CONTEXT_2;
#endif
      } else {
        addr_contexts[i].used = 0;
      }
#else
      addr_contexts[i].used = 0;
#endif /* SICSLOWPAN_CONF_ADDR_CONTEXT_1 */

    }
  }
#endif /* SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS > 1 */

#endif /* SICSLOWPAN_COMPRESSION == SICSLOWPAN_COMPRESSION_HC06 */

}

const struct compression sicslowpan_compression = {
	.init = init,
	.compress = compress,
	.uncompress = uncompress
};
