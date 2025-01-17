/**
 * \addtogroup uip6
 * @{
 */

/**
 * \file
 *         The uIP TCP/IPv6 stack code.
 *
 * \author Adam Dunkels <adam@sics.se>
 * \author Julien Abeille <jabeille@cisco.com> (IPv6 related code)
 * \author Mathilde Durvy <mdurvy@cisco.com> (IPv6 related code)
 */
/*
 * Copyright (c) 2001-2003, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack.
 *
 * $Id: uip6.c,v 1.25 2011/01/04 22:11:37 joxe Exp $
 *
 */

/*
 * uIP is a small implementation of the IP, UDP and TCP protocols (as
 * well as some basic ICMP stuff). The implementation couples the IP,
 * UDP, TCP and the application layers very tightly. To keep the size
 * of the compiled code down, this code frequently uses the goto
 * statement. While it would be possible to break the uip_process()
 * function into many smaller functions, this would increase the code
 * size because of the overhead of parameter passing and the fact that
 * the optimier would not be as efficient.
 *
 * The principle is that we have a small buffer, called the uip_buf,
 * in which the device driver puts an incoming packet. The TCP/IP
 * stack parses the headers in the packet, and calls the
 * application. If the remote host has sent data to the application,
 * this data is present in the uip_buf and the application read the
 * data from there. It is up to the application to put this data into
 * a byte stream if needed. The application will not be fed with data
 * that is out of sequence.
 *
 * If the application whishes to send data to the peer, it should put
 * its data into the uip_buf. The uip_appdata pointer points to the
 * first available byte. The TCP/IP stack will calculate the
 * checksums, and fill in the necessary header fields and finally send
 * the packet back to the peer.
 */
/******************************************************************************
 *
 * Copyright (c) 2014 Texas Instruments Inc.  All rights reserved.
 *
 * DESCRIPTION:
 *
 * HISTORY:
 *
 *
 ******************************************************************************/
#include "net/uip.h"
#include "net/uipopt.h"
#include "net/uip-icmp6.h"
#include "net/uip-nd6.h"
#include "net/uip-ds6.h"

#include "uip_rpl_process.h"

#include <string.h>

#if UIP_CONF_IPV6
/*---------------------------------------------------------------------------*/
/* For Debug, logging, statistics                                            */
/*---------------------------------------------------------------------------*/

#ifdef DEBUG  
#undef DEBUG
#endif
#define DEBUG DEBUG_NONE
#include "net/uip-debug.h"

#if UIP_LOGGING == 1
#include <stdio.h>
void uip_log(char *msg);
#define UIP_LOG(m) uip_log(m)
#else
#define UIP_LOG(m)
#endif /* UIP_LOGGING == 1 */

#if UIP_STATISTICS == 1
struct uip_stats uip_stat;
#endif /* UIP_STATISTICS == 1 */

typedef struct __uip_appcall_dbg_
{
    uint16_t tcp_uip_poll;
    uint16_t uip_timeout;
    uint16_t uip_timeout_sync;
    uint16_t uip_rexmit;
    uint16_t uip_poll_new_data;
    uint16_t uip_abort;
    uint16_t uip_syn_rcvd;
    uint16_t uip_syn_sent;
    uint16_t conn_failure_uip_abort;
    uint16_t uip_newdata;
    uint16_t uip_newdata_ackdata;
    uint16_t uip_last_ack_close;
    uint16_t uip_fin_wait_1_close;
    uint16_t uip_fin_wait_2_close;

} UIP_APPCALL_DBG_s;

UIP_APPCALL_DBG_s UIP_APPCALL_Dbg;

/*---------------------------------------------------------------------------*/
/** @{ \name Layer 2 variables */
/*---------------------------------------------------------------------------*/
/** Host L2 address */
#if UIP_CONF_LL_802154
uip_lladdr_t uip_lladdr;
#else /*UIP_CONF_LL_802154*/
uip_lladdr_t uip_lladdr =
{
    {   0x00,0x06,0x98,0x00,0x02,0x32}};
#endif /*UIP_CONF_LL_802154*/
/** @} */

/*---------------------------------------------------------------------------*/
/** @{ \name Layer 3 variables */
/*---------------------------------------------------------------------------*/
/**
 * \brief Type of the next header in IPv6 header or extension headers
 *
 * Can be the next header field in the IPv6 header or in an extension header.
 * When doing fragment reassembly, we must change the value of the next header
 * field in the header before the fragmentation header, hence we need a pointer
 * to this field.
 */
//u8_t *uip_next_hdr;
char *uip_next_hdr;
/** \brief bitmap we use to record which IPv6 headers we have already seen */
u16_t uip_ext_bitmap = 0;
/**
 * \brief length of the extension headers read. updated each time we process
 * a header
 */
u16_t uip_ext_len = 0;
/** \brief length of the header options read */
u16_t uip_ext_opt_offset = 0;
/** @} */

/*---------------------------------------------------------------------------*/
/* Buffers                                                                   */
/*---------------------------------------------------------------------------*/
/** \name Buffer defines
 *  @{
 */
#define FBUF                             ((struct uip_tcpip_hdr *)&uip_reassbuf[0])
#define UIP_IP_BUF                          ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_ICMP_BUF                      ((struct uip_icmp_hdr *)&uip_buf[uip_l2_l3_hdr_len])
#define UIP_UDP_BUF                        ((struct uip_udp_hdr *)&uip_buf[UIP_LLH_LEN + UIP_IPH_LEN])
#define UIP_TCP_BUF                        ((struct uip_tcp_hdr *)&uip_buf[UIP_LLH_LEN + UIP_IPH_LEN])
#define UIP_EXT_BUF                        ((struct uip_ext_hdr *)&uip_buf[uip_l2_l3_hdr_len])
#define UIP_ROUTING_BUF                ((struct uip_routing_hdr *)&uip_buf[uip_l2_l3_hdr_len])
#define UIP_FRAG_BUF                      ((struct uip_frag_hdr *)&uip_buf[uip_l2_l3_hdr_len])
#define UIP_HBHO_BUF                      ((struct uip_hbho_hdr *)&uip_buf[uip_l2_l3_hdr_len])
#define UIP_DESTO_BUF                    ((struct uip_desto_hdr *)&uip_buf[uip_l2_l3_hdr_len])
#define UIP_EXT_HDR_OPT_BUF            ((struct uip_ext_hdr_opt *)&uip_buf[uip_l2_l3_hdr_len + uip_ext_opt_offset])
#define UIP_EXT_HDR_OPT_PADN_BUF  ((struct uip_ext_hdr_opt_padn *)&uip_buf[uip_l2_l3_hdr_len + uip_ext_opt_offset])
#define UIP_ICMP6_ERROR_BUF            ((struct uip_icmp6_error *)&uip_buf[uip_l2_l3_icmp_hdr_len])
/** @} */
/** \name Buffer variables
 *  @{
 */
/** Packet buffer for incoming and outgoing packets */
#ifndef UIP_CONF_EXTERNAL_BUFFER

uint16_t uip_len_buff[MAX_NUM_OUTSTANDING_UIP_PACKETS];
uip_buf_t uip_aligned_buf_space[MAX_NUM_OUTSTANDING_UIP_PACKETS];
uip_lladdr_t uip_src_lladdr_buff[MAX_NUM_OUTSTANDING_UIP_PACKETS];
uip_lladdr_t uip_dst_lladdr_buff[MAX_NUM_OUTSTANDING_UIP_PACKETS];
uip_buf_t *uip_aligned_buf = uip_aligned_buf_space;
uint8_t uipBuffer_ReadIdx = MAX_NUM_OUTSTANDING_UIP_PACKETS - 1;
uint8_t uipBuffer_WriteIdx = 0;
volatile uint8_t uipBuffer_WriteLock = 0;
uip_buf_t appRxFifo[MAX_NUM_OUTSTANDING_APP_RX_PACKETS];
uint16_t appRxLenFifo[MAX_NUM_OUTSTANDING_APP_RX_PACKETS];
uint8_t appRxFifo_ReadIdx = MAX_NUM_OUTSTANDING_APP_RX_PACKETS - 1;
uint8_t appRxFifo_WriteIdx = 0;
uint16_t uipRxPacketLossCount;
uint16_t uipTxPacketLossCount;
/*--------------------------------------------------------------------*/
void uipBuffer_LockWriteBuffer()
{
    while (uipBuffer_WriteLock)
    {
#ifdef LINUX_GATEWAY
        usleep(1000);
#else
        Task_sleep(CLOCK_SECOND / 1000);
#endif
    }

    uipBuffer_WriteLock = 1;
}

/*--------------------------------------------------------------------*/
void uipBuffer_UnlockWriteBuffer()
{
    uipBuffer_WriteLock = 0;
}
/*--------------------------------------------------------------------*/
void uipBuffer_GetInputPacket()
{
    uipBuffer_ReadIdx = (uipBuffer_ReadIdx + 1)
            % MAX_NUM_OUTSTANDING_UIP_PACKETS;
}
/*--------------------------------------------------------------------*/
void uipBufferIndex_inc(uint8_t *idx_p)
{
    *idx_p = (*idx_p + 1) % MAX_NUM_OUTSTANDING_UIP_PACKETS;
}

/*--------------------------------------------------------------------*/
void appRxFifoIndex_inc(uint8_t *idx_p)
{
    *idx_p = (*idx_p + 1) % MAX_NUM_OUTSTANDING_APP_RX_PACKETS;
}
#endif /* UIP_CONF_EXTERNAL_BUFFER */

uint16_t uip_app_datalen;

/* The uip_appdata pointer points to application data. */
void *uip_appdata;
/* The uip_appdata pointer points to the application data which is to be sent*/
void *uip_sappdata;

#if UIP_URGDATA > 0
/* The uip_urgdata pointer points to urgent data (out-of-band data), if present */
void *uip_urgdata;
u16_t uip_urglen, uip_surglen;
#endif /* UIP_URGDATA > 0 */

/* The uip_len is either 8 or 16 bits, depending on the maximum packet size.*/
u16_t uip_slen;
/** @} */

/*---------------------------------------------------------------------------*/
/** @{ \name General variables                                               */
/*---------------------------------------------------------------------------*/

/* The uip_flags variable is used for communication between the TCP/IP stack
 and the application program. */
u16_t uip_flags;

/* uip_conn always points to the current connection (set to NULL for UDP). */
struct uip_conn *uip_conn;

/* Temporary variables. */
#if (UIP_TCP || UIP_UDP)
static u16_t c;
#endif

#if UIP_ACTIVE_OPEN || UIP_UDP
/* Keeps track of the last port used for a new connection. */
static u16_t lastport;
#endif /* UIP_ACTIVE_OPEN || UIP_UDP */
/** @} */

/*---------------------------------------------------------------------------*/
/* TCP                                                                       */
/*---------------------------------------------------------------------------*/
/** \name TCP defines
 *@{
 */
/* Structures and definitions. */
#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10
#define TCP_URG 0x20
#define TCP_CTL 0x3f

#define TCP_OPT_END     0   /* End of TCP options list */
#define TCP_OPT_NOOP    1   /* "No-operation" TCP option */
#define TCP_OPT_MSS     2   /* Maximum segment size TCP option */

#define TCP_OPT_MSS_LEN 4   /* Length of TCP MSS option. */
/** @} */
/** \name TCP variables
 *@{
 */
#if UIP_TCP
/* The uip_conns array holds all TCP connections. */
struct uip_conn uip_conns[UIP_CONNS];

/* The uip_listenports list all currently listning ports. */
uint16_t uip_listenports[UIP_LISTENPORTS];

/* The iss variable is used for the TCP initial sequence number. */
static uint8_t iss[4];

/* Temporary variables. */
uint8_t uip_acc32[4];
static uint8_t opt;
static uint16_t tmp16;
#endif /* UIP_TCP */
/** @} */

/*---------------------------------------------------------------------------*/
/** @{ \name UDP variables                                                   */
/*---------------------------------------------------------------------------*/
#if UIP_UDP
struct uip_udp_conn *uip_udp_conn;
struct uip_udp_conn uip_udp_conns[UIP_UDP_CONNS];
#endif /* UIP_UDP */
/** @} */

/*---------------------------------------------------------------------------*/
/** @{ \name ICMPv6 variables                                                */
/*---------------------------------------------------------------------------*/
#if UIP_CONF_ICMP6
/** single possible icmpv6 "connection" */
struct uip_icmp6_conn uip_icmp6_conns;
#endif /*UIP_CONF_ICMP6*/

// Robert Liang:  TCPIP is using this toprint IPv6 address
// disable this one
//#define PRINT6ADDR

/*---------------------------------------------------------------------------*/
/* Functions                                                                 */
/*---------------------------------------------------------------------------*/
#if (!UIP_ARCH_ADD32 && UIP_TCP)
void
uip_add32(uint8_t *op32, uint16_t op16)
{
    uip_acc32[3] = op32[3] + (op16 & 0xff);
    uip_acc32[2] = op32[2] + (op16 >> 8);
    uip_acc32[1] = op32[1];
    uip_acc32[0] = op32[0];

    if(uip_acc32[2] < (op16 >> 8))
    {
        ++uip_acc32[1];
        if(uip_acc32[1] == 0)
        {
            ++uip_acc32[0];
        }
    }

    if(uip_acc32[3] < (op16 & 0xff))
    {
        ++uip_acc32[2];
        if(uip_acc32[2] == 0)
        {
            ++uip_acc32[1];
            if(uip_acc32[1] == 0)
            {
                ++uip_acc32[0];
            }
        }
    }
}

#endif /* UIP_ARCH_ADD32 && UIP_TCP */

#if ! UIP_ARCH_CHKSUM
/*---------------------------------------------------------------------------*/
static u16_t chksum(u16_t sum, const char *data, u16_t len) //length is in byte units
{
    u16_t t;
    int i = 0;

    len -= 1;

    while (i < len)
    { /* At least two more bytes */
        t = (pltfrm_byte_get((void*)data,i)<< 8) + pltfrm_byte_get((void*)data,i+1);
        sum += t;
        if(sum < t)
        {
            sum++; /* carry */
        }
        i += 2;
    }

    if (i == len)
    {
        t = (pltfrm_byte_get((void*)data,i)<< 8) + 0;
        sum += t;
        if(sum < t)
        {
            sum++; /* carry */
        }
    }

    /* Return sum in host byte order. */
    return sum;
}

/*---------------------------------------------------------------------------*/
u16_t uip_chksum(u16_t *data, u16_t len)
{
    return uip_htons(chksum(0, (char *) data, len));
}
/*---------------------------------------------------------------------------*/
#ifndef UIP_ARCH_IPCHKSUM
u16_t
uip_ipchksum(void)
{
    u16_t sum;

    sum = chksum(0, &uip_buf[UIP_LLH_LEN], UIP_IPH_LEN);
    LOG_INFO("uip_ipchksum: sum 0x%04x\n", sum);
    return (sum == 0) ? 0xffff : uip_htons(sum);
}
#endif
/*---------------------------------------------------------------------------*/
static u16_t upper_layer_chksum(u16_t proto)
{
    /* gcc 4.4.0 - 4.6.1 (maybe 4.3...) with -Os on 8 bit CPUS incorrectly compiles:
     * int bar (int);
     * int foo (unsigned char a, unsigned char b) {
     *   int len = (a << 8) + b; //len becomes 0xff00&<random>+b
     *   return len + bar (len);
     * }
     * upper_layer_len triggers this bug unless it is declared volatile.
     * See https://sourceforge.net/apps/mantisbt/contiki/view.php?id=3
     */
    volatile u16_t upper_layer_len;
    u16_t sum;

    //upper_layer_len = (((u16_t)(UIP_IP_BUF->len[0]) << 8) + UIP_IP_BUF->len[1] - uip_ext_len);
    upper_layer_len = (((u16_t) (UIP_IP_BUF->len0) << 8) + UIP_IP_BUF->len1
            - BYTE_UNITS(uip_ext_len)); //uip_ext_len is always in CHAR_BIT units. upper_layer_len must be in BYTE units

    PRINTF("Upper layer checksum len: %d from: %d\n", upper_layer_len,
            BYTE_UNITS(UIP_IPH_LEN + UIP_LLH_LEN + uip_ext_len));

    /* First sum pseudoheader. */
    /* IP protocol and length fields. This addition cannot carry. */
    sum = upper_layer_len + proto;
    /* Sum IP source and destination addresses. */
    sum = chksum(sum, (char *) &UIP_IP_BUF->srcipaddr,
                 2 * BYTE_UNITS(sizeof(uip_ipaddr_t))); //the chksum function needs the length in byte units

    /* Sum TCP header and data. */
    sum = chksum(sum, &uip_buf[UIP_IPH_LEN + UIP_LLH_LEN + uip_ext_len],
                 upper_layer_len);

    return (sum == 0) ? 0xffff : uip_htons(sum);
}
/*---------------------------------------------------------------------------*/
u16_t uip_icmp6chksum(void)
{
    return upper_layer_chksum(UIP_PROTO_ICMP6);

}
/*---------------------------------------------------------------------------*/
#if UIP_TCP
u16_t
uip_tcpchksum(void)
{
    return upper_layer_chksum(UIP_PROTO_TCP);
}
#endif /* UIP_TCP */
/*---------------------------------------------------------------------------*/
#if UIP_UDP && UIP_UDP_CHECKSUMS
u16_t uip_udpchksum(void)
{
    return upper_layer_chksum(UIP_PROTO_UDP);
}
#endif /* UIP_UDP && UIP_UDP_CHECKSUMS */
#endif /* UIP_ARCH_CHKSUM */
/*---------------------------------------------------------------------------*/
void uip_init(void)
{
    uip_ds6_init();

#if UIP_TCP
    for(c = 0; c < UIP_LISTENPORTS; ++c)
    {
        uip_listenports[c] = 0;
    }
    for(c = 0; c < UIP_CONNS; ++c)
    {
        uip_conns[c].tcpstateflags = UIP_CLOSED;
    }
#endif /* UIP_TCP */

#if UIP_ACTIVE_OPEN || UIP_UDP
    lastport = 1024;
#endif /* UIP_ACTIVE_OPEN || UIP_UDP */

#if UIP_UDP
    for (c = 0; c < UIP_UDP_CONNS; ++c)
    {
        uip_udp_conns[c].lport = 0;
    }
#endif /* UIP_UDP */

}
/*---------------------------------------------------------------------------*/
#if UIP_TCP && UIP_ACTIVE_OPEN
struct uip_conn *
uip_connect(uip_ipaddr_t *ripaddr, uint16_t rport)
{
    register struct uip_conn *conn, *cconn;

    /* Find an unused local port. */
    again:
    ++lastport;

    if(lastport >= 32000)
    {
        lastport = 4096;
    }

    /* Check if this port is already in use, and if so try to find
     another one. */
    for(c = 0; c < UIP_CONNS; ++c)
    {
        conn = &uip_conns[c];
        if(conn->tcpstateflags != UIP_CLOSED &&
                conn->lport == uip_htons(lastport))
        {
            goto again;
        }
    }

    conn = 0;
    for(c = 0; c < UIP_CONNS; ++c)
    {
        cconn = &uip_conns[c];
        if(cconn->tcpstateflags == UIP_CLOSED)
        {
            conn = cconn;
            break;
        }
        if(cconn->tcpstateflags == UIP_TIME_WAIT)
        {
            if(conn == 0 ||
                    cconn->timer > conn->timer)
            {
                conn = cconn;
            }
        }
    }

    if(conn == 0)
    {
        return 0;
    }

    conn->tcpstateflags = UIP_SYN_SENT;

    conn->snd_nxt[0] = iss[0];
    conn->snd_nxt[1] = iss[1];
    conn->snd_nxt[2] = iss[2];
    conn->snd_nxt[3] = iss[3];

    conn->rcv_nxt[0] = 0;
    conn->rcv_nxt[1] = 0;
    conn->rcv_nxt[2] = 0;
    conn->rcv_nxt[3] = 0;

    conn->initialmss = conn->mss = UIP_TCP_MSS;

    conn->len = 1; /* TCP length of the SYN is one. */
    conn->nrtx = 0;
    conn->timer = 1; /* Send the SYN next time around. */
    conn->rto = UIP_RTO;
    conn->sa = 0;
    conn->sv = 16; /* Initial value of the RTT variance. */
    conn->lport = uip_htons(lastport);
    conn->rport = rport;
    uip_ipaddr_copy(&conn->ripaddr, ripaddr);

    return conn;
}
#endif /* UIP_TCP && UIP_ACTIVE_OPEN */
/*---------------------------------------------------------------------------*/
void remove_ext_hdr(void)
{
    /* Remove ext header before TCP/UDP processing. */
    if (uip_ext_len > 0)
    {
        LOG_INFO2("Cutting ext-header before processing (extlen: %d, uiplen: %d)\n",
                uip_ext_len, uip_len);
        //if(uip_len < UIP_IPH_LEN + uip_ext_len) {
        if (uip_len< BYTE_UNITS(UIP_IPH_LEN + uip_ext_len))
        {
            PRINTF("ERROR: uip_len too short compared to ext len\n");
            uip_ext_len = 0;
            uip_len = 0;
            return;
        }
        //memmove(((u8_t *)UIP_TCP_BUF), (u8_t *)UIP_TCP_BUF + uip_ext_len, uip_len - UIP_IPH_LEN - uip_ext_len);
        memmove(((char *) UIP_TCP_BUF), (char *) UIP_TCP_BUF + uip_ext_len,
        CHAR_BIT_UNITS(uip_len) - UIP_IPH_LEN - uip_ext_len);

        //uip_len -= uip_ext_len;
        uip_len-= BYTE_UNITS(uip_ext_len);

        /* Update the IP length. */
        UIP_IP_BUF->len0 = (uip_len- BYTE_UNITS(UIP_IPH_LEN)) >> 8;
        UIP_IP_BUF->len1 = (uip_len- BYTE_UNITS(UIP_IPH_LEN)) & 0xff;
        uip_ext_len = 0;
    }
}
/*---------------------------------------------------------------------------*/
#if UIP_UDP
struct uip_udp_conn *
uip_udp_new(const uip_ipaddr_t *ripaddr, u16_t rport)
{
    register struct uip_udp_conn *conn;

    /* Find an unused local port. */
    again: ++lastport;

    if (lastport >= 32000)
    {
        lastport = 4096;
    }

    for (c = 0; c < UIP_UDP_CONNS; ++c)
    {
        if (uip_udp_conns[c].lport == uip_htons(lastport))
        {
            goto again;
        }
    }

    conn = 0;
    for (c = 0; c < UIP_UDP_CONNS; ++c)
    {
        if (uip_udp_conns[c].lport == 0)
        {
            conn = &uip_udp_conns[c];
            break;
        }
    }

    if (conn == 0)
    {
        return 0;
    }

    conn->lport = UIP_HTONS(lastport);
    conn->rport = rport;
    if (ripaddr == NULL)
    {
        memset(&conn->ripaddr, 0, sizeof(uip_ipaddr_t));
    }
    else
    {
        uip_ipaddr_copy(&conn->ripaddr, ripaddr);
    }
    conn->ttl = uip_ds6_if.cur_hop_limit;

    return conn;
}
#endif /* UIP_UDP */
/*---------------------------------------------------------------------------*/
#if UIP_TCP
void
uip_unlisten(uint16_t port)
{
    for(c = 0; c < UIP_LISTENPORTS; ++c)
    {
        if(uip_listenports[c] == port)
        {
            uip_listenports[c] = 0;
            return;
        }
    }
}
/*---------------------------------------------------------------------------*/
void
uip_listen(uint16_t port)
{
    for(c = 0; c < UIP_LISTENPORTS; ++c)
    {
        if(uip_listenports[c] == 0)
        {
            uip_listenports[c] = port;
            return;
        }
    }
}
#endif
/*---------------------------------------------------------------------------*/

#if UIP_CONF_IPV6_REASSEMBLY
#define UIP_REASS_BUFSIZE (UIP_BUFSIZE - UIP_LLH_LEN)

static uint8_t uip_reassbuf[UIP_REASS_BUFSIZE];

static uint8_t uip_reassbitmap[UIP_REASS_BUFSIZE / (8 * 8)];
/*the first byte of an IP fragment is aligned on an 8-byte boundary */

static const uint8_t bitmap_bits[8] =
{   0xff, 0x7f, 0x3f, 0x1f,
    0x0f, 0x07, 0x03, 0x01};
static uint16_t uip_reasslen;
static uint8_t uip_reassflags;

#define UIP_REASS_FLAG_LASTFRAG 0x01
#define UIP_REASS_FLAG_FIRSTFRAG 0x02
#define UIP_REASS_FLAG_ERROR_MSG 0x04

/*
 * See RFC 2460 for a description of fragmentation in IPv6
 * A typical Ipv6 fragment
 *  +------------------+--------+--------------+
 *  |  Unfragmentable  |Fragment|    first     |
 *  |       Part       | Header |   fragment   |
 *  +------------------+--------+--------------+
 */

struct etimer uip_reass_timer; /* timer for reassembly */
uint8_t uip_reass_on; /* equal to 1 if we are currently reassembling a packet */

static uint32_t uip_id; /* For every packet that is to be fragmented, the source
 node generates an Identification value that is present
 in all the fragments */
#define IP_MF   0x0001

static uint16_t
uip_reass(void)
{
    uint16_t offset=0;
    uint16_t len;
    uint16_t i;

    /* If ip_reasstmr is zero, no packet is present in the buffer */
    /* We first write the unfragmentable part of IP header into the reassembly
     buffer. The reset the other reassembly variables. */
    if(uip_reass_on == 0)
    {
        LOG_INFO("Starting reassembly\n");
        memcpy(FBUF, UIP_IP_BUF, uip_ext_len + UIP_IPH_LEN);
        /* temporary in case we do not receive the fragment with offset 0 first */
        etimer_set(&uip_reass_timer, UIP_REASS_MAXAGE*CLOCK_SECOND);
        uip_reass_on = 1;
        uip_reassflags = 0;
        uip_id = UIP_FRAG_BUF->id;
        /* Clear the bitmap. */
        memset(uip_reassbitmap, 0, sizeof(uip_reassbitmap));
    }
    /*
     * Check if the incoming fragment matches the one currently present
     * in the reasembly buffer. If so, we proceed with copying the fragment
     * into the buffer.
     */
    if(uip_ipaddr_cmp(&FBUF->srcipaddr, &UIP_IP_BUF->srcipaddr) &&
            uip_ipaddr_cmp(&FBUF->destipaddr, &UIP_IP_BUF->destipaddr) &&
            UIP_FRAG_BUF->id == uip_id)
    {
        len = uip_len - uip_ext_len - UIP_IPH_LEN - UIP_FRAGH_LEN;
        offset = (uip_ntohs(UIP_FRAG_BUF->offsetresmore) & 0xfff8);
        /* in byte, originaly in multiple of 8 bytes*/
        LOG_INFO("len %d\n", len);
        LOG_INFO("offset %d\n", offset);
        if(offset == 0)
        {
            uip_reassflags |= UIP_REASS_FLAG_FIRSTFRAG;
            /*
             * The Next Header field of the last header of the Unfragmentable
             * Part is obtained from the Next Header field of the first
             * fragment's Fragment header.
             */
            *uip_next_hdr = UIP_FRAG_BUF->next;
            memcpy(FBUF, UIP_IP_BUF, uip_ext_len + UIP_IPH_LEN);
            LOG_INFO("src ");
            PRINT6ADDR(&FBUF->srcipaddr);
            LOG_INFO("dest ");
            PRINT6ADDR(&FBUF->destipaddr);
            LOG_INFO("next %d\n", UIP_IP_BUF->proto);

        }

        /* If the offset or the offset + fragment length overflows the
         reassembly buffer, we discard the entire packet. */
        if(offset > UIP_REASS_BUFSIZE ||
                offset + len > UIP_REASS_BUFSIZE)
        {
            uip_reass_on = 0;
            etimer_stop(&uip_reass_timer);
            return 0;
        }

        /* If this fragment has the More Fragments flag set to zero, it is the
         last fragment*/
        if((uip_ntohs(UIP_FRAG_BUF->offsetresmore) & IP_MF) == 0)
        {
            uip_reassflags |= UIP_REASS_FLAG_LASTFRAG;
            /*calculate the size of the entire packet*/
            uip_reasslen = offset + len;
            LOG_INFO("LAST FRAGMENT reasslen %d\n", uip_reasslen);
        }
        else
        {
            /* If len is not a multiple of 8 octets and the M flag of that fragment
             is 1, then that fragment must be discarded and an ICMP Parameter
             Problem, Code 0, message should be sent to the source of the fragment,
             pointing to the Payload Length field of the fragment packet. */
            if(len % 8 != 0)
            {
                uip_icmp6_error_output(ICMP6_PARAM_PROB, ICMP6_PARAMPROB_HEADER, 4);
                uip_reassflags |= UIP_REASS_FLAG_ERROR_MSG;
                /* not clear if we should interrupt reassembly, but it seems so from
                 the conformance tests */
                uip_reass_on = 0;
                etimer_stop(&uip_reass_timer);
                return uip_len;
            }
        }

        /* Copy the fragment into the reassembly buffer, at the right
         offset. */
        memcpy((uint8_t *)FBUF + UIP_IPH_LEN + uip_ext_len + offset,
                (uint8_t *)UIP_FRAG_BUF + UIP_FRAGH_LEN, len);

        /* Update the bitmap. */
        if(offset >> 6 == (offset + len) >> 6)
        {
            uip_reassbitmap[offset >> 6] |=
            bitmap_bits[(offset >> 3) & 7] &
            ~bitmap_bits[((offset + len) >> 3) & 7];
        }
        else
        {
            /* If the two endpoints are in different bytes, we update the
             bytes in the endpoints and fill the stuff inbetween with
             0xff. */
            uip_reassbitmap[offset >> 6] |= bitmap_bits[(offset >> 3) & 7];

            for(i = (1 + (offset >> 6)); i < ((offset + len) >> 6); ++i)
            {
                uip_reassbitmap[i] = 0xff;
            }
            uip_reassbitmap[(offset + len) >> 6] |=
            ~bitmap_bits[((offset + len) >> 3) & 7];
        }

        /* Finally, we check if we have a full packet in the buffer. We do
         this by checking if we have the last fragment and if all bits
         in the bitmap are set. */

        if(uip_reassflags & UIP_REASS_FLAG_LASTFRAG)
        {
            /* Check all bytes up to and including all but the last byte in
             the bitmap. */
            for(i = 0; i < (uip_reasslen >> 6); ++i)
            {
                if(uip_reassbitmap[i] != 0xff)
                {
                    return 0;
                }
            }
            /* Check the last byte in the bitmap. It should contain just the
             right amount of bits. */
            if(uip_reassbitmap[uip_reasslen >> 6] !=
                    (uint8_t)~bitmap_bits[(uip_reasslen >> 3) & 7])
            {
                return 0;
            }

            /* If we have come this far, we have a full packet in the
             buffer, so we copy it to uip_buf. We also reset the timer. */
            uip_reass_on = 0;
            etimer_stop(&uip_reass_timer);

            uip_reasslen += UIP_IPH_LEN + uip_ext_len;
            memcpy(UIP_IP_BUF, FBUF, uip_reasslen);
            UIP_IP_BUF->len[0] = ((uip_reasslen - UIP_IPH_LEN) >> 8);
            UIP_IP_BUF->len[1] = ((uip_reasslen - UIP_IPH_LEN) & 0xff);
            LOG_INFO("REASSEMBLED PAQUET %d (%d)\n", uip_reasslen,
                    (UIP_IP_BUF->len[0] << 8) | UIP_IP_BUF->len[1]);

            return uip_reasslen;

        }
    }
    else
    {
        LOG_INFO("Already reassembling another paquet\n");
    }
    return 0;
}

void
uip_reass_over(void)
{
    /* to late, we abandon the reassembly of the packet */

    uip_reass_on = 0;
    etimer_stop(&uip_reass_timer);

    if(uip_reassflags & UIP_REASS_FLAG_FIRSTFRAG)
    {
        LOG_INFO("FRAG INTERRUPTED TOO LATE\n");
        /* If the first fragment has been received, an ICMP Time Exceeded
         -- Fragment Reassembly Time Exceeded message should be sent to the
         source of that fragment. */
        /** \note
         * We don't have a complete packet to put in the error message.
         * We could include the first fragment but since its not mandated by
         * any RFC, we decided not to include it as it reduces the size of
         * the packet.
         */
        uip_len = 0;
        uip_ext_len = 0;
        memcpy(UIP_IP_BUF, FBUF, UIP_IPH_LEN); /* copy the header for src
         and dest address*/
        uip_icmp6_error_output(ICMP6_TIME_EXCEEDED, ICMP6_TIME_EXCEED_REASSEMBLY, 0);

        UIP_STAT(++uip_stat.ip.sent);
        uip_flags = 0;
    }
}

#endif /* UIP_CONF_IPV6_REASSEMBLY */

/*---------------------------------------------------------------------------*/
#if UIP_TCP
static void
uip_add_rcv_nxt(uint16_t n)
{
    uip_add32(uip_conn->rcv_nxt, n);
    uip_conn->rcv_nxt[0] = uip_acc32[0];
    uip_conn->rcv_nxt[1] = uip_acc32[1];
    uip_conn->rcv_nxt[2] = uip_acc32[2];
    uip_conn->rcv_nxt[3] = uip_acc32[3];
}
#endif
/*---------------------------------------------------------------------------*/

/**
 * \brief Process the options in Destination and Hop By Hop extension headers
 */
static u16_t ext_hdr_options_process(void)
{
    int res;
    int opt_offset;
    /*
     * Length field in the extension header: length of the header in units of
     * 8 bytes, excluding the first 8 bytes
     * length field in an option : the length of data in the option
     */
    uip_ext_opt_offset = CHAR_BIT_UNITS(2);
    while (uip_ext_opt_offset < CHAR_BIT_UNITS((UIP_EXT_BUF->len << 3) + 8))
    {
        switch (UIP_EXT_HDR_OPT_BUF->type)
        {
        /*
         * for now we do not support any options except padding ones
         * PAD1 does not make sense as the header must be 8bytes aligned,
         * hence we can only have
         */
        case UIP_EXT_HDR_OPT_PAD1:
            PRINTF("Processing PAD1 option");
            uip_ext_opt_offset += CHAR_BIT_UNITS(1);
            break;
        case UIP_EXT_HDR_OPT_PADN:
            PRINTF("Processing PADN option");
            uip_ext_opt_offset += CHAR_BIT_UNITS(
                    UIP_EXT_HDR_OPT_PADN_BUF->opt_len + 2);
            break;

        default:
            opt_offset = uip_ext_opt_offset;
            res = UIP_HBHO_HDR_EXTENSION(UIP_HBHO_HDR_VERIFY, &opt_offset);
            if (res == 1)
            {
                //Verified
                uip_ext_opt_offset += CHAR_BIT_UNITS(
                        (UIP_EXT_HDR_OPT_BUF->len) + 2);
                return 0;
            }
            else if (res == -1)
            {
                //Processed extension header with error
                return 1;
            }
            /*
             * check the two highest order bits of the option
             * - 00 skip over this option and continue processing the header.
             * - 01 discard the packet.
             * - 10 discard the packet and, regardless of whether or not the
             *   packet's Destination Address was a multicast address, send an
             *   ICMP Parameter Problem, Code 2, message to the packet's
             *   Source Address, pointing to the unrecognized Option Type.
             * - 11 discard the packet and, only if the packet's Destination
             *   Address was not a multicast address, send an ICMP Parameter
             *   Problem, Code 2, message to the packet's Source Address,
             *   pointing to the unrecognized Option Type.
             */
            LOG_INFO1("MSB %x\n", UIP_EXT_HDR_OPT_BUF->type);
            switch (UIP_EXT_HDR_OPT_BUF->type & 0xC0)
            {
            case 0:
                break;
            case 0x40:
                return 1;
            case 0xC0:
                if (uip_is_addr_mcast(&UIP_IP_BUF->destipaddr))
                {
                    return 1;
                }
            case 0x80:
                uip_icmp6_error_output(
                        ICMP6_PARAM_PROB,
                        ICMP6_PARAMPROB_OPTION,
                        (u32_t) BYTE_UNITS(
                                UIP_IPH_LEN + uip_ext_len + uip_ext_opt_offset));
                return 2;
            }
            /* in the cases were we did not discard, update ext_opt* */
            uip_ext_opt_offset += CHAR_BIT_UNITS(UIP_EXT_HDR_OPT_BUF->len + 2);
            break;
        }
    }
    return 0;
}

/*---------------------------------------------------------------------------*/
void uip_process(u16_t flag)
{
#if UIP_TCP
    register struct uip_conn *uip_connr = uip_conn;
#endif /* UIP_TCP */
#if UIP_UDP
    if (flag == UIP_UDP_SEND_CONN)
    {
        goto udp_send;
    }
#endif /* UIP_UDP */
    uip_sappdata = uip_appdata = &uip_buf[UIP_IPTCPH_LEN + UIP_LLH_LEN];

    /* Check if we were invoked because of a poll request for a
     particular connection. */
    if (flag == UIP_POLL_REQUEST)
    {
#if UIP_TCP
        if((uip_connr->tcpstateflags & UIP_TS_MASK) == UIP_ESTABLISHED &&
                !uip_outstanding(uip_connr))
        {
            uip_flags = UIP_POLL;
            UIP_APPCALL_Dbg.tcp_uip_poll++;

            UIP_APPCALL();
            goto appsend;
#if UIP_ACTIVE_OPEN
        }
        else if((uip_connr->tcpstateflags & UIP_TS_MASK) == UIP_SYN_SENT)
        {
            /* In the SYN_SENT state, we retransmit out SYN. */
            UIP_TCP_BUF->flags = 0;
            goto tcp_send_syn;
#endif /* UIP_ACTIVE_OPEN */
        }
        goto drop;
#endif /* UIP_TCP */
        /* Check if we were invoked because of the perodic timer fireing. */
    }
    else if (flag == UIP_TIMER)
    {
        /* Reset the length variables. */
#if UIP_TCP
        uip_len = 0;
        uip_slen = 0;

        /* Increase the initial sequence number. */
        if(++iss[3] == 0)
        {
            if(++iss[2] == 0)
            {
                if(++iss[1] == 0)
                {
                    ++iss[0];
                }
            }
        }

        /*
         * Check if the connection is in a state in which we simply wait
         * for the connection to time out. If so, we increase the
         * connection's timer and remove the connection if it times
         * out.
         */
        if(uip_connr->tcpstateflags == UIP_TIME_WAIT ||
                uip_connr->tcpstateflags == UIP_FIN_WAIT_2)
        {
            ++(uip_connr->timer);
            if(uip_connr->timer == UIP_TIME_WAIT_TIMEOUT)
            {
                uip_connr->tcpstateflags = UIP_CLOSED;
            }
        }
        else if(uip_connr->tcpstateflags != UIP_CLOSED)
        {
            /*
             * If the connection has outstanding data, we increase the
             * connection's timer and see if it has reached the RTO value
             * in which case we retransmit.
             */
            if(uip_outstanding(uip_connr))
            {
                if(uip_connr->timer-- == 0)
                {
                    if(uip_connr->nrtx == UIP_MAXRTX ||
                            ((uip_connr->tcpstateflags == UIP_SYN_SENT ||
                                            uip_connr->tcpstateflags == UIP_SYN_RCVD) &&
                                    uip_connr->nrtx == UIP_MAXSYNRTX))
                    {
                        uip_connr->tcpstateflags = UIP_CLOSED;

                        /*
                         * We call UIP_APPCALL() with uip_flags set to
                         * UIP_TIMEDOUT to inform the application that the
                         * connection has timed out.
                         */
                        uip_flags = UIP_TIMEDOUT;

                        if ( (uip_connr->tcpstateflags == UIP_SYN_SENT ) ||
                                (uip_connr->tcpstateflags == UIP_SYN_RCVD) )
                        {
                            UIP_APPCALL_Dbg.uip_timeout_sync++;
                        }
                        else
                        UIP_APPCALL_Dbg.uip_timeout++;

                        UIP_APPCALL();

                        /* We also send a reset packet to the remote host. */
                        UIP_TCP_BUF->flags = TCP_RST | TCP_ACK;
                        goto tcp_send_nodata;
                    }

#if 0
                    /* Exponential backoff. */
                    uip_connr->timer = UIP_RTO << (uip_connr->nrtx > 4?
                            4:
                            uip_connr->nrtx);
#endif
                    // Robert, disable exponetial back off
                    uip_connr->timer = UIP_RTO;
                    ++(uip_connr->nrtx);

                    /*
                     * Ok, so we need to retransmit. We do this differently
                     * depending on which state we are in. In ESTABLISHED, we
                     * call upon the application so that it may prepare the
                     * data for the retransmit. In SYN_RCVD, we resend the
                     * SYNACK that we sent earlier and in LAST_ACK we have to
                     * retransmit our FINACK.
                     */
                    UIP_STAT(++uip_stat.tcp.rexmit);
                    switch(uip_connr->tcpstateflags & UIP_TS_MASK)
                    {
                        case UIP_SYN_RCVD:
                        /* In the SYN_RCVD state, we should retransmit our SYNACK. */
                        goto tcp_send_synack;

#if UIP_ACTIVE_OPEN
                        case UIP_SYN_SENT:
                        /* In the SYN_SENT state, we retransmit out SYN. */
                        UIP_TCP_BUF->flags = 0;
                        goto tcp_send_syn;
#endif /* UIP_ACTIVE_OPEN */

                        case UIP_ESTABLISHED:
                        /*
                         * In the ESTABLISHED state, we call upon the application
                         * to do the actual retransmit after which we jump into
                         * the code for sending out the packet (the apprexmit
                         * label).
                         */
                        uip_flags = UIP_REXMIT;
                        UIP_APPCALL_Dbg.uip_rexmit++;

                        UIP_APPCALL();
                        goto apprexmit;

                        case UIP_FIN_WAIT_1:
                        case UIP_CLOSING:
                        case UIP_LAST_ACK:
                        /* In all these states we should retransmit a FINACK. */
                        goto tcp_send_finack;
                    }
                }
            }
            else if((uip_connr->tcpstateflags & UIP_TS_MASK) == UIP_ESTABLISHED)
            {
                /*
                 * If there was no need for a retransmission, we poll the
                 * application for new data.
                 */
                uip_flags = UIP_POLL;
                UIP_APPCALL_Dbg.uip_poll_new_data++;

                UIP_APPCALL();
                goto appsend;
            }
        }
        goto drop;
#endif /* UIP_TCP */
    }
#if UIP_UDP
    if (flag == UIP_UDP_TIMER)
    {
        if (uip_udp_conn->lport != 0)
        {
            uip_conn = NULL;
            uip_sappdata = uip_appdata = &uip_buf[UIP_IPUDPH_LEN + UIP_LLH_LEN];
            uip_len= uip_slen = 0;
            uip_flags = UIP_POLL;
            UIP_UDP_APPCALL();
            goto udp_send;
        }
        else
        {
            goto drop;
        }
    }
#endif /* UIP_UDP */

    /* This is where the input processing starts. */
    UIP_STAT(++uip_stat.ip.recv);
    UIP_STAT_RX_IP_NUM_INC;
    /* Start of IP input header processing code. */

    /* Check validity of the IP header. */
    if ((UIP_IP_BUF->vtc & 0xf0) != 0x60)
    { /* IP version and header length. */
        UIP_STAT(++uip_stat.ip.drop); UIP_STAT(++uip_stat.ip.vhlerr); UIP_LOG("ipv6: invalid version.");
        UIP_STAT_RX_IP_DROP_NUM_INC;
        goto drop;
    }
    /*
     * Check the size of the packet. If the size reported to us in
     * uip_len is smaller the size reported in the IP header, we assume
     * that the packet has been corrupted in transit. If the size of
     * uip_len is larger than the size reported in the IP packet header,
     * the packet has been padded and we set uip_len to the correct
     * value..
     */

    if ((UIP_IP_BUF->len0 << 8) + UIP_IP_BUF->len1 <= uip_len)
    {
        uip_len = (UIP_IP_BUF->len0 << 8) + UIP_IP_BUF->len1 + BYTE_UNITS(UIP_IPH_LEN);
        /*
         * The length reported in the IPv6 header is the
         * length of the payload that follows the
         * header. However, uIP uses the uip_len variable
         * for holding the size of the entire packet,
         * including the IP header. For IPv4 this is not a
         * problem as the length field in the IPv4 header
         * contains the length of the entire packet. But
         * for IPv6 we need to add the size of the IPv6
         * header (40 bytes).
         */
    }
    else
    {
        UIP_LOG("ip: packet shorter than reported in IP header.");
        UIP_STAT_RX_IP_DROP_NUM_INC;
        goto drop;
    }

    PRINTF("IPv6 packet received from "); PRINT6ADDR(&UIP_IP_BUF->srcipaddr); PRINTF(" to "); PRINT6ADDR(&UIP_IP_BUF->destipaddr); PRINTF("");

    if (uip_is_addr_mcast(&UIP_IP_BUF->srcipaddr))
    {
        UIP_STAT(++uip_stat.ip.drop); PRINTF("Dropping packet, src is mcast");
        UIP_STAT_RX_IP_DROP_NUM_INC;
        goto drop;
    }

#if UIP_CONF_ROUTER
    /*
     * Next header field processing. In IPv6, we can have extension headers,
     * if present, the Hop-by-Hop Option must be processed before forwarding
     * the packet.
     */
    //uip_next_hdr = &UIP_IP_BUF->proto;
    uip_next_hdr = ((char*) UIP_IP_BUF + CHAR_BIT_UNITS(6)); //proto is the 6th byte in the header
    uip_ext_len = 0;
    uip_ext_bitmap = 0;
    if (((struct uip_ext_hdr *) uip_next_hdr)->next == UIP_PROTO_HBHO)
    {
#if UIP_CONF_IPV6_CHECKS
        uip_ext_bitmap |= UIP_EXT_HDR_BITMAP_HBHO;
#endif /*UIP_CONF_IPV6_CHECKS*/
        switch (ext_hdr_options_process())
        {
        case 0:
            /* continue */
            uip_next_hdr = (char*) UIP_EXT_BUF;
            uip_ext_len += CHAR_BIT_UNITS((UIP_EXT_BUF->len << 3) + 8);
            break;
        case 1:
            PRINTF("Dropping packet after extension header processing\n");
            /* silently discard */
            UIP_STAT_RX_IP_DROP_NUM_INC;
            goto drop;
        case 2:
            PRINTF("Sending error message after extension header processing\n");
            /* send icmp error message (created in ext_hdr_options_process)
             * and discard*/
            goto send;
        }
    }

    /* TBD Some Parameter problem messages */
    if (!uip_ds6_is_my_addr(&UIP_IP_BUF->destipaddr) &&
    !uip_ds6_is_my_maddr(&UIP_IP_BUF->destipaddr))
    {
        if (!uip_is_addr_mcast(&UIP_IP_BUF->destipaddr) &&
        !uip_is_addr_link_local(&UIP_IP_BUF->destipaddr) &&
        !uip_is_addr_link_local(&UIP_IP_BUF->srcipaddr) &&
        !uip_is_addr_unspecified(&UIP_IP_BUF->srcipaddr) &&
        !uip_is_addr_loopback(&UIP_IP_BUF->destipaddr))
        {

            /* Check MTU */
            if (uip_len> UIP_LINK_MTU)
            {
                uip_icmp6_error_output(ICMP6_PACKET_TOO_BIG, 0, UIP_LINK_MTU);
                UIP_STAT(++uip_stat.ip.drop);
                UIP_STAT_RX_IP_DROP_NUM_INC;
                goto send;
            }
            /* Check Hop Limit */
            if(UIP_IP_BUF->ttl <= 1)
            {
                uip_icmp6_error_output(ICMP6_TIME_EXCEEDED,
                ICMP6_TIME_EXCEED_TRANSIT, 0);
                UIP_STAT(++uip_stat.ip.drop);
                UIP_STAT_RX_IP_DROP_NUM_INC;
                goto send;
            }
            //If using hop by hop option header, update it
            UIP_HBHO_HDR_EXTENSION(UIP_HBHO_HDR_UPDATE_EMPTY, NULL);

            UIP_IP_BUF->ttl = UIP_IP_BUF->ttl - 1;
            PRINTF("Forwarding packet to ");
            PRINT6ADDR(&UIP_IP_BUF->destipaddr);
            PRINTF("");
            UIP_STAT(++uip_stat.ip.forwarded);
            UIP_STAT_RX_IP_ROUTED_NUM_INC;
            goto send;
        }
        else
        {
            if((uip_is_addr_link_local(&UIP_IP_BUF->srcipaddr)) &&
            (!uip_is_addr_unspecified(&UIP_IP_BUF->srcipaddr)) &&
            (!uip_is_addr_loopback(&UIP_IP_BUF->destipaddr)) &&
            (!uip_is_addr_mcast(&UIP_IP_BUF->destipaddr)) &&
            (!uip_ds6_is_addr_onlink((&UIP_IP_BUF->destipaddr))))
            {
                PRINTF("LL source address with off link destination, dropping\n");
                UIP_STAT_RX_IP_DROP_NUM_INC;
                uip_icmp6_error_output(ICMP6_DST_UNREACH,
                ICMP6_DST_UNREACH_NOTNEIGHBOR, 0);
                goto send;
            }
            LOG_INFO("Dropping packet, not for me and link local or multicast\n");
            UIP_STAT(++uip_stat.ip.drop);
            UIP_STAT_RX_IP_DROP_NUM_INC;
            goto drop;
        }
    }
#else /* UIP_CONF_ROUTER */
    if(!uip_ds6_is_my_addr(&UIP_IP_BUF->destipaddr) &&
    !uip_ds6_is_my_maddr(&UIP_IP_BUF->destipaddr) &&
    !uip_is_addr_mcast(&UIP_IP_BUF->destipaddr))
    {
        LOG_INFO("Dropping packet, not for me\n");
        UIP_STAT(++uip_stat.ip.drop);
        UIP_STAT_RX_IP_DROP_NUM_INC;
        goto drop;
    }

    /*
     * Next header field processing. In IPv6, we can have extension headers,
     * they are processed here
     */
    uip_next_hdr = &UIP_IP_BUF->proto;
    uip_ext_len = 0;
    uip_ext_bitmap = 0;
#endif /* UIP_CONF_ROUTER */

    while (1)
    {
        //switch(*uip_next_hdr){
        switch (((struct uip_ext_hdr *) uip_next_hdr)->next)
        {
#if UIP_TCP
        case UIP_PROTO_TCP:
        /* TCP, for both IPv4 and IPv6 */
        goto tcp_input;
#endif /* UIP_TCP */
#if UIP_UDP
        case UIP_PROTO_UDP:
            /* UDP, for both IPv4 and IPv6 */
            goto udp_input;
#endif /* UIP_UDP */
        case UIP_PROTO_ICMP6:
            /* ICMPv6 */
            goto icmp6_input;
        case UIP_PROTO_HBHO:
            LOG_INFO("Processing hbh header\n");
            /* Hop by hop option header */
#if UIP_CONF_IPV6_CHECKS
            /* Hop by hop option header. If we saw one HBH already, drop */
            if (uip_ext_bitmap & UIP_EXT_HDR_BITMAP_HBHO)
            {
                goto bad_hdr;
            }
            else
            {
                uip_ext_bitmap |= UIP_EXT_HDR_BITMAP_HBHO;
            }
#endif /*UIP_CONF_IPV6_CHECKS*/
            switch (ext_hdr_options_process())
            {
            case 0:
                /*continue*/
                uip_next_hdr = (char*) UIP_EXT_BUF;
                uip_ext_len += CHAR_BIT_UNITS((UIP_EXT_BUF->len << 3) + 8);
                break;
            case 1:
                UIP_STAT_RX_IP_DROP_NUM_INC;
                /*silently discard*/
                goto drop;
            case 2:
                /* send icmp error message (created in ext_hdr_options_process)
                 * and discard*/
                UIP_STAT_RX_IP_DROP_NUM_INC;
                goto send;
            }
            break;
        case UIP_PROTO_DESTO:
#if UIP_CONF_IPV6_CHECKS
            /* Destination option header. if we saw two already, drop */
            LOG_INFO("Processing desto header\n");
            if (uip_ext_bitmap & UIP_EXT_HDR_BITMAP_DESTO1)
            {
                if (uip_ext_bitmap & UIP_EXT_HDR_BITMAP_DESTO2)
                {
                    goto bad_hdr;
                }
                else
                {
                    uip_ext_bitmap |= UIP_EXT_HDR_BITMAP_DESTO2;
                }
            }
            else
            {
                uip_ext_bitmap |= UIP_EXT_HDR_BITMAP_DESTO1;
            }
#endif /*UIP_CONF_IPV6_CHECKS*/
            switch (ext_hdr_options_process())
            {
            case 0:
                /*continue*/
                uip_next_hdr = (char*) UIP_EXT_BUF;
                uip_ext_len += CHAR_BIT_UNITS((UIP_EXT_BUF->len << 3) + 8);
                break;
            case 1:
                UIP_STAT_RX_IP_DROP_NUM_INC;
                /*silently discard*/
                goto drop;
            case 2:
                /* send icmp error message (created in ext_hdr_options_process)
                 * and discard*/
                UIP_STAT_RX_IP_DROP_NUM_INC;
                goto send;
            }
            break;
        case UIP_PROTO_ROUTING:
#if UIP_CONF_IPV6_CHECKS
            /* Routing header. If we saw one already, drop */
            if (uip_ext_bitmap & UIP_EXT_HDR_BITMAP_ROUTING)
            {
                goto bad_hdr;
            }
            else
            {
                uip_ext_bitmap |= UIP_EXT_HDR_BITMAP_ROUTING;
            }
#endif /*UIP_CONF_IPV6_CHECKS*/
            /*
             * Routing Header  length field is in units of 8 bytes, excluding
             * As per RFC2460 section 4.4, if routing type is unrecognized:
             * if segments left = 0, ignore the header
             * if segments left > 0, discard packet and send icmp error pointing
             * to the routing type
             */

            LOG_INFO("Processing Routing header\n");
            if (UIP_ROUTING_BUF->seg_left > 0)
            {
                /*
                 uip_icmp6_error_output(ICMP6_PARAM_PROB, ICMP6_PARAMPROB_HEADER, UIP_IPH_LEN + uip_ext_len + 2);
                 UIP_STAT(++uip_stat.ip.drop);
                 UIP_LOG("ip6: unrecognized routing type");
                 goto send;
                 */
                switch (UIP_SRC_ROUTING_HDR_EXTENSION(
                        UIP_SOURCE_ROUTING_HDR_PROCESS, NULL))
                {
                case UIP_SOURCE_ROUTING_HDR_DISCARD:
                    UIP_STAT_RX_IP_DROP_NUM_INC;
                    goto drop;
                case UIP_SOURCE_ROUTING_HDR_ERROR_MSG:
                    UIP_STAT_RX_IP_DROP_NUM_INC;
                case UIP_SOURCE_ROUTING_HDR_FWD_MSG:
                    goto send;
                }
            }
            uip_next_hdr = (char*) UIP_EXT_BUF;
            uip_ext_len += CHAR_BIT_UNITS((UIP_EXT_BUF->len << 3) + 8);
            break;
        case UIP_PROTO_FRAG:
            /* Fragmentation header:call the reassembly function, then leave */
#if UIP_CONF_IPV6_REASSEMBLY
            LOG_INFO("Processing frag header\n");
            uip_len = uip_reass();
            if(uip_len == 0)
            {
                goto drop;
            }
            if(uip_reassflags & UIP_REASS_FLAG_ERROR_MSG)
            {
                /* we are not done with reassembly, this is an error message */
                goto send;
            }
            /*packet is reassembled, reset the next hdr to the beginning
             of the IP header and restart the parsing of the reassembled pkt*/
            LOG_INFO("Processing reassembled packet\n");
            uip_ext_len = 0;
            uip_ext_bitmap = 0;
            uip_next_hdr = &UIP_IP_BUF->proto;
            break;
#else /* UIP_CONF_IPV6_REASSEMBLY */
            UIP_STAT(++uip_stat.ip.drop);
            UIP_STAT(++uip_stat.ip.fragerr);
            UIP_LOG("ip: fragment dropped.");
            UIP_STAT_RX_IP_DROP_NUM_INC;
            goto drop;
#endif /* UIP_CONF_IPV6_REASSEMBLY */
        case UIP_PROTO_NONE:
            UIP_STAT_RX_IP_DROP_NUM_INC;
            goto drop;
        default:
            goto bad_hdr;
        }
    }
    bad_hdr:
    /*
     * RFC 2460 send error message parameterr problem, code unrecognized
     * next header, pointing to the next header field
     */
    uip_icmp6_error_output(
            ICMP6_PARAM_PROB, ICMP6_PARAMPROB_NEXTHEADER,
            (u32_t) BYTE_UNITS((int)(uip_next_hdr - (char *)UIP_IP_BUF)));
    UIP_STAT(++uip_stat.ip.drop); UIP_STAT(++uip_stat.ip.protoerr); UIP_LOG("ip6: unrecognized header");
    UIP_STAT_RX_IP_DROP_NUM_INC;
    goto send;
    /* End of headers processing */

    icmp6_input:
    /* This is IPv6 ICMPv6 processing code. */
    LOG_INFO2("icmp6_input: length %d type: %d \n", uip_len, UIP_ICMP_BUF->type);
    UIP_STAT_RX_ICMP_NUM_INC;

#if UIP_CONF_IPV6_CHECKS
    /* Compute and check the ICMP header checksum */
    if (uip_icmp6chksum() != 0xffff)
    {
        UIP_STAT(++uip_stat.icmp.drop); UIP_STAT(++uip_stat.icmp.chkerr); UIP_LOG("icmpv6: bad checksum."); PRINTF("icmpv6: bad checksum.");
        UIP_STAT_RX_ICMP_CHKERR_NUM_INC;
        UIP_STAT_RX_ICMP_DROP_NUM_INC;
        goto drop;
    }
#endif /*UIP_CONF_IPV6_CHECKS*/

    UIP_STAT(++uip_stat.icmp.recv);
    /*
     * Here we process incoming ICMPv6 packets
     * For echo request, we send echo reply
     * For ND pkts, we call the appropriate function in uip-nd6.c
     * We do not treat Error messages for now
     * If no pkt is to be sent as an answer to the incoming one, we
     * "goto drop". Else we just break; then at the after the "switch"
     * we "goto send"
     */
#if UIP_CONF_ICMP6
    UIP_ICMP6_APPCALL(UIP_ICMP_BUF->type);
#endif /*UIP_CONF_ICMP6*/

    switch (UIP_ICMP_BUF->type)
    {
    case ICMP6_NS:
#if UIP_ND6_SEND_NA
        UIP_STAT_RX_NS_NUM_INC;
        uip_nd6_ns_input();
#else /* UIP_ND6_SEND_NA */
        UIP_STAT(++uip_stat.icmp.drop);
        uip_len = 0;
#endif /* UIP_ND6_SEND_NA */
        break;
    case ICMP6_NA:
#if UIP_ND6_SEND_NA
        UIP_STAT_RX_NA_NUM_INC;
        uip_nd6_na_input();
#else /* UIP_ND6_SEND_NA */
        UIP_STAT(++uip_stat.icmp.drop);
        uip_len = 0;
#endif /* UIP_ND6_SEND_NA */
        break;
    case ICMP6_RS:
#if UIP_CONF_ROUTER && UIP_ND6_SEND_RA
        uip_nd6_rs_input();
#else /* UIP_CONF_ROUTER && UIP_ND6_SEND_RA */
        UIP_STAT(++uip_stat.icmp.drop);
        UIP_STAT_RX_ICMP_DROP_NUM_INC;
        uip_len= 0;
#endif /* UIP_CONF_ROUTER && UIP_ND6_SEND_RA */
        break;
        case ICMP6_RA:
#if UIP_CONF_ROUTER
        UIP_STAT(++uip_stat.icmp.drop);
        UIP_STAT_RX_ICMP_DROP_NUM_INC;
        uip_len = 0;
#else /* UIP_CONF_ROUTER */
        uip_nd6_ra_input();
#endif /* UIP_CONF_ROUTER */
        break;
        case ICMP6_ECHO_REQUEST:
        UIP_STAT_RX_ECHO_REQ_NUM_INC;
        uip_icmp6_echo_request_input();
        break;
        case ICMP6_ECHO_REPLY:
        LOG_INFO("Received an icmp6 echo reply\n");
        UIP_STAT(++uip_stat.icmp.recv);
        UIP_STAT_RX_ECHO_REP_NUM_INC;
        UIP_ECHO_REPLY_INDICATION((char*)UIP_IP_BUF, uip_datalen());
        uip_len = 0;
        break;
        default:
#ifdef UIP_ICMP6_INPUT_INDICATION
        if (UIP_ICMP6_INPUT_INDICATION(UIP_ICMP_BUF->type))
        return;
#endif
        LOG_INFO1("Unknown icmp6 message type %d\n", UIP_ICMP_BUF->type);
        UIP_STAT(++uip_stat.icmp.drop);
        UIP_STAT(++uip_stat.icmp.typeerr);
        UIP_LOG("icmp6: unknown ICMP message.");
        UIP_STAT_RX_ICMP_DROP_NUM_INC;
        uip_len = 0;
        break;
    }

    if (uip_len> 0)
    {
        goto send;
    }
    else
    {
        goto drop;
    }
    /* End of IPv6 ICMP processing. */

#if UIP_UDP
    /* UDP input processing. */
    udp_input:

    remove_ext_hdr();

    //change PROTO to UDP in case we have to forward packet using SERIAL (FALLBACK_INTERFACE)
    UIP_IP_BUF->proto = UIP_PROTO_UDP;

    PRINTF("Receiving UDP packet\n"); UIP_STAT(++uip_stat.udp.recv);
    UIP_STAT_RX_UDP_NUM_INC;

    /* UDP processing is really just a hack. We don't do anything to the
     UDP/IP headers, but let the UDP application do all the hard
     work. If the application sets uip_slen, it has a packet to
     send. */
#if UIP_UDP_CHECKSUMS
    uip_len= uip_len - BYTE_UNITS(UIP_IPUDPH_LEN);
    uip_appdata = &uip_buf[UIP_IPUDPH_LEN + UIP_LLH_LEN];
    /* XXX hack: UDP/IPv6 receivers should drop packets with UDP
     checksum 0. Here, we explicitly receive UDP packets with checksum
     0. This is to be able to debug code that for one reason or
     another miscomputes UDP checksums. The reception of zero UDP
     checksums should be turned into a configration option. */
    if (UIP_UDP_BUF->udpchksum != 0 && uip_udpchksum() != 0xffff)
    {
        UIP_STAT(++uip_stat.udp.drop); UIP_STAT(++uip_stat.udp.chkerr); PRINTF("udp: bad checksum 0xX04x 0xX04x");
        UIP_STAT_RX_UDP_DROP_NUM_INC;
        UIP_STAT_RX_UDP_CHKERR_NUM_INC;
        goto drop;
    }
#else /* UIP_UDP_CHECKSUMS */
    uip_len = uip_len - BYTE_UNITS(UIP_IPUDPH_LEN);
#endif /* UIP_UDP_CHECKSUMS */
    /* Make sure that the UDP destination port number is not zero. */
    if (UIP_UDP_BUF->destport == 0)
    {
        PRINTF("udp: zero port.");
        UIP_STAT_RX_UDP_DROP_NUM_INC;
        goto drop;
    }

    /* Demultiplex this UDP packet between the UDP "connections". */
    for (uip_udp_conn = &uip_udp_conns[0];
            uip_udp_conn < &uip_udp_conns[UIP_UDP_CONNS]; ++uip_udp_conn)
    {
        /* If the local UDP port is non-zero, the connection is considered
         to be used. If so, the local port number is checked against the
         destination port number in the received packet. If the two port
         numbers match, the remote port number is checked if the
         connection is bound to a remote port. Finally, if the
         connection is bound to a remote IP address, the source IP
         address of the packet is checked. */

//if(uip_udp_conn->lport == 0 ||
        if (uip_udp_conn->lport == UIP_UDP_LISTEN_ALL_PORT || //In this case we "hack" the UDP connections. Any connection listening to port UIP_UDP_LISTEN_ALL_PORT will receive all data regardless the remote port or ip address
                (((uip_udp_conn->lport != 0)
                        && (UIP_UDP_BUF->destport == uip_udp_conn->lport))
                        && ((uip_udp_conn->rport == 0)
                                || (UIP_UDP_BUF->srcport == uip_udp_conn->rport))
                        && (uip_is_addr_unspecified(&uip_udp_conn->ripaddr)
                                || uip_ipaddr_cmp(&UIP_IP_BUF->srcipaddr,
                                                  &uip_udp_conn->ripaddr))))
        {
            goto udp_found;
        }
    } LOG_INFO("udp: no matching connection found\n");

#if UIP_UDP_SEND_UNREACH_NOPORT
    uip_icmp6_error_output(ICMP6_DST_UNREACH, ICMP6_DST_UNREACH_NOPORT, 0);
    UIP_STAT(++uip_stat.ip.drop);
    goto send;
#else
    UIP_STAT_RX_UDP_DROP_NUM_INC;
    goto drop;
#endif

    udp_found: LOG_INFO("In udp_found\n");

    uip_conn = NULL;
    uip_flags = UIP_NEWDATA;
    uip_sappdata = uip_appdata = &uip_buf[UIP_IPUDPH_LEN + UIP_LLH_LEN];
    uip_slen = 0;
    UIP_UDP_APPCALL();

    udp_send: LOG_INFO("In udp_send\n");

    if (uip_slen == 0)
    {
        goto drop;
    }
    uip_len= uip_slen + BYTE_UNITS(UIP_IPUDPH_LEN);

    /* For IPv6, the IP length field does not include the IPv6 IP header
     length. */
    UIP_IP_BUF->len0 = ((uip_len- BYTE_UNITS(UIP_IPH_LEN)) >> 8);
    UIP_IP_BUF->len1 = ((uip_len- BYTE_UNITS(UIP_IPH_LEN)) & 0xff);

    UIP_IP_BUF->ttl = uip_udp_conn->ttl;
    UIP_IP_BUF->proto = UIP_PROTO_UDP;

    UIP_UDP_BUF->udplen = UIP_HTONS(uip_slen + BYTE_UNITS(UIP_UDPH_LEN));
    UIP_UDP_BUF->udpchksum = 0;

    UIP_UDP_BUF->srcport = uip_udp_conn->lport;
    UIP_UDP_BUF->destport = uip_udp_conn->rport;

    uip_ipaddr_copy(&UIP_IP_BUF->destipaddr, &uip_udp_conn->ripaddr);
    uip_ds6_select_src(&UIP_IP_BUF->srcipaddr, &UIP_IP_BUF->destipaddr);

    /*(DO NOT SEND UDP MESSAGE IF WE ONLY HAVE UNSPECIFIED ADDRESS)*/
    if (uip_is_addr_unspecified(&UIP_IP_BUF->srcipaddr))
    {
        goto drop;
    }

    uip_appdata = &uip_buf[UIP_LLH_LEN + UIP_IPTCPH_LEN];

#if UIP_UDP_CHECKSUMS
    /* Calculate UDP checksum. */
    UIP_UDP_BUF->udpchksum = ~(uip_udpchksum());
    if (UIP_UDP_BUF->udpchksum == 0)
    {
        UIP_UDP_BUF->udpchksum = 0xffff;
    }
#endif /* UIP_UDP_CHECKSUMS */

    //If using hop by hop option header, insert it
    UIP_HBHO_HDR_EXTENSION(UIP_HBHO_HDR_INSERT, NULL);

    UIP_STAT(++uip_stat.udp.sent);
    UIP_STAT_TX_UDP_NUM_INC;
    goto ip_send_nolen;
#endif /* UIP_UDP */

#if UIP_TCP
    /* TCP input processing. */
    tcp_input:

    remove_ext_hdr();

    UIP_STAT(++uip_stat.tcp.recv);
    LOG_INFO("Receiving TCP packet\n");
    /* Start of TCP input header processing code. */

    if(uip_tcpchksum() != 0xffff)
    { /* Compute and check the TCP
     checksum. */
        UIP_STAT(++uip_stat.tcp.drop);
        UIP_STAT(++uip_stat.tcp.chkerr);
        LOG_INFO("tcp: bad checksum 0x%04x 0x%04x\n", UIP_TCP_BUF->tcpchksum,
                uip_tcpchksum());
        goto drop;
    }

    /* Make sure that the TCP port number is not zero. */
    if(UIP_TCP_BUF->destport == 0 || UIP_TCP_BUF->srcport == 0)
    {
        LOG_INFO("tcp: zero port.");
        goto drop;
    }

    /* Demultiplex this segment. */
    /* First check any active connections. */
    for(uip_connr = &uip_conns[0]; uip_connr <= &uip_conns[UIP_CONNS - 1];
            ++uip_connr)
    {
        if(uip_connr->tcpstateflags != UIP_CLOSED &&
                UIP_TCP_BUF->destport == uip_connr->lport &&
                UIP_TCP_BUF->srcport == uip_connr->rport &&
                uip_ipaddr_cmp(&UIP_IP_BUF->srcipaddr, &uip_connr->ripaddr))
        {
            goto found;
        }
    }

    /* If we didn't find and active connection that expected the packet,
     either this packet is an old duplicate, or this is a SYN packet
     destined for a connection in LISTEN. If the SYN flag isn't set,
     it is an old packet and we send a RST. */
    if((UIP_TCP_BUF->flags & TCP_CTL) != TCP_SYN)
    {
        goto reset;
    }

    tmp16 = UIP_TCP_BUF->destport;
    /* Next, check listening connections. */
    for(c = 0; c < UIP_LISTENPORTS; ++c)
    {
        if(tmp16 == uip_listenports[c])
        {
            goto found_listen;
        }
    }

    /* No matching connection found, so we send a RST packet. */
    UIP_STAT(++uip_stat.tcp.synrst);

    reset:
    LOG_INFO("In reset\n");
    /* We do not send resets in response to resets. */
    if(UIP_TCP_BUF->flags & TCP_RST)
    {
        goto drop;
    }

    UIP_STAT(++uip_stat.tcp.rst);

    UIP_TCP_BUF->flags = TCP_RST | TCP_ACK;
    uip_len = UIP_IPTCPH_LEN;
    UIP_TCP_BUF->tcpoffset = 5 << 4;

    /* Flip the seqno and ackno fields in the TCP header. */
    c = UIP_TCP_BUF->seqno[3];
    UIP_TCP_BUF->seqno[3] = UIP_TCP_BUF->ackno[3];
    UIP_TCP_BUF->ackno[3] = c;

    c = UIP_TCP_BUF->seqno[2];
    UIP_TCP_BUF->seqno[2] = UIP_TCP_BUF->ackno[2];
    UIP_TCP_BUF->ackno[2] = c;

    c = UIP_TCP_BUF->seqno[1];
    UIP_TCP_BUF->seqno[1] = UIP_TCP_BUF->ackno[1];
    UIP_TCP_BUF->ackno[1] = c;

    c = UIP_TCP_BUF->seqno[0];
    UIP_TCP_BUF->seqno[0] = UIP_TCP_BUF->ackno[0];
    UIP_TCP_BUF->ackno[0] = c;

    /* We also have to increase the sequence number we are
     acknowledging. If the least significant byte overflowed, we need
     to propagate the carry to the other bytes as well. */
    if(++UIP_TCP_BUF->ackno[3] == 0)
    {
        if(++UIP_TCP_BUF->ackno[2] == 0)
        {
            if(++UIP_TCP_BUF->ackno[1] == 0)
            {
                ++UIP_TCP_BUF->ackno[0];
            }
        }
    }

    /* Swap port numbers. */
    tmp16 = UIP_TCP_BUF->srcport;
    UIP_TCP_BUF->srcport = UIP_TCP_BUF->destport;
    UIP_TCP_BUF->destport = tmp16;

    /* Swap IP addresses. */
    uip_ipaddr_copy(&UIP_IP_BUF->destipaddr, &UIP_IP_BUF->srcipaddr);
    uip_ds6_select_src(&UIP_IP_BUF->srcipaddr, &UIP_IP_BUF->destipaddr);
    /* And send out the RST packet! */
    goto tcp_send_noconn;

    /* This label will be jumped to if we matched the incoming packet
     with a connection in LISTEN. In that case, we should create a new
     connection and send a SYNACK in return. */
    found_listen:
    LOG_INFO("In found listen\n");
    /* First we check if there are any connections avaliable. Unused
     connections are kept in the same table as used connections, but
     unused ones have the tcpstate set to CLOSED. Also, connections in
     TIME_WAIT are kept track of and we'll use the oldest one if no
     CLOSED connections are found. Thanks to Eddie C. Dost for a very
     nice algorithm for the TIME_WAIT search. */
    uip_connr = 0;
    for(c = 0; c < UIP_CONNS; ++c)
    {
        if(uip_conns[c].tcpstateflags == UIP_CLOSED)
        {
            uip_connr = &uip_conns[c];
            break;
        }
        if(uip_conns[c].tcpstateflags == UIP_TIME_WAIT)
        {
            if(uip_connr == 0 ||
                    uip_conns[c].timer > uip_connr->timer)
            {
                uip_connr = &uip_conns[c];
            }
        }
    }

    if(uip_connr == 0)
    {
        /* All connections are used already, we drop packet and hope that
         the remote end will retransmit the packet at a time when we
         have more spare connections. */
        UIP_STAT(++uip_stat.tcp.syndrop);
        UIP_LOG("tcp: found no unused connections.");
        goto drop;
    }
    uip_conn = uip_connr;

    /* Fill in the necessary fields for the new connection. */
    uip_connr->rto = uip_connr->timer = UIP_RTO;
    uip_connr->sa = 0;
    uip_connr->sv = 4;
    uip_connr->nrtx = 0;
    uip_connr->lport = UIP_TCP_BUF->destport;
    uip_connr->rport = UIP_TCP_BUF->srcport;
    uip_ipaddr_copy(&uip_connr->ripaddr, &UIP_IP_BUF->srcipaddr);
    uip_connr->tcpstateflags = UIP_SYN_RCVD;

    uip_connr->snd_nxt[0] = iss[0];
    uip_connr->snd_nxt[1] = iss[1];
    uip_connr->snd_nxt[2] = iss[2];
    uip_connr->snd_nxt[3] = iss[3];
    uip_connr->len = 1;

    /* rcv_nxt should be the seqno from the incoming packet + 1. */
    uip_connr->rcv_nxt[3] = UIP_TCP_BUF->seqno[3];
    uip_connr->rcv_nxt[2] = UIP_TCP_BUF->seqno[2];
    uip_connr->rcv_nxt[1] = UIP_TCP_BUF->seqno[1];
    uip_connr->rcv_nxt[0] = UIP_TCP_BUF->seqno[0];
    uip_add_rcv_nxt(1);

    /* Parse the TCP MSS option, if present. */
    if((UIP_TCP_BUF->tcpoffset & 0xf0) > 0x50)
    {
        for(c = 0; c < ((UIP_TCP_BUF->tcpoffset >> 4) - 5) << 2;)
        {
            opt = uip_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + c];
            if(opt == TCP_OPT_END)
            {
                /* End of options. */
                break;
            }
            else if(opt == TCP_OPT_NOOP)
            {
                ++c;
                /* NOP option. */
            }
            else if(opt == TCP_OPT_MSS &&
                    uip_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + 1 + c] == TCP_OPT_MSS_LEN)
            {
                /* An MSS option with the right option length. */
                tmp16 = ((uint16_t)uip_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + 2 + c] << 8) |
                (uint16_t)uip_buf[UIP_IPTCPH_LEN + UIP_LLH_LEN + 3 + c];
                uip_connr->initialmss = uip_connr->mss =
                tmp16 > UIP_TCP_MSS? UIP_TCP_MSS: tmp16;

                /* And we are done processing options. */
                break;
            }
            else
            {
                /* All other options have a length field, so that we easily
                 can skip past them. */
                if(uip_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + 1 + c] == 0)
                {
                    /* If the length field is zero, the options are malformed
                     and we don't process them further. */
                    break;
                }
                c += uip_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + 1 + c];
            }
        }
    }

    /* Our response will be a SYNACK. */
#if UIP_ACTIVE_OPEN
    tcp_send_synack:
    UIP_TCP_BUF->flags = TCP_ACK;

    tcp_send_syn:
    UIP_TCP_BUF->flags |= TCP_SYN;
#else /* UIP_ACTIVE_OPEN */
    tcp_send_synack:
    UIP_TCP_BUF->flags = TCP_SYN | TCP_ACK;
#endif /* UIP_ACTIVE_OPEN */

    /* We send out the TCP Maximum Segment Size option with our
     SYNACK. */
    UIP_TCP_BUF->optdata[0] = TCP_OPT_MSS;
    UIP_TCP_BUF->optdata[1] = TCP_OPT_MSS_LEN;
    UIP_TCP_BUF->optdata[2] = (UIP_TCP_MSS) / 256;
    UIP_TCP_BUF->optdata[3] = (UIP_TCP_MSS) & 255;
    uip_len = UIP_IPTCPH_LEN + TCP_OPT_MSS_LEN;
    UIP_TCP_BUF->tcpoffset = ((UIP_TCPH_LEN + TCP_OPT_MSS_LEN) / 4) << 4;
    goto tcp_send;

    /* This label will be jumped to if we found an active connection. */
    found:
    LOG_INFO("In found\n");
    uip_conn = uip_connr;
    uip_flags = 0;
    /* We do a very naive form of TCP reset processing; we just accept
     any RST and kill our connection. We should in fact check if the
     sequence number of this reset is wihtin our advertised window
     before we accept the reset. */
    if(UIP_TCP_BUF->flags & TCP_RST)
    {
        uip_connr->tcpstateflags = UIP_CLOSED;
        UIP_LOG("tcp: got reset, aborting connection.");
        uip_flags = UIP_ABORT;
        UIP_APPCALL_Dbg.uip_abort++;
        UIP_APPCALL();
        goto drop;
    }
    /* Calculate the length of the data, if the application has sent
     any data to us. */
    c = (UIP_TCP_BUF->tcpoffset >> 4) << 2;
    /* uip_len will contain the length of the actual TCP data. This is
     calculated by subtracing the length of the TCP header (in
     c) and the length of the IP header (20 bytes). */
    uip_len = uip_len - c - UIP_IPH_LEN;

    /* First, check if the sequence number of the incoming packet is
     what we're expecting next. If not, we send out an ACK with the
     correct numbers in, unless we are in the SYN_RCVD state and
     receive a SYN, in which case we should retransmit our SYNACK
     (which is done futher down). */
    if(!((((uip_connr->tcpstateflags & UIP_TS_MASK) == UIP_SYN_SENT) &&
                            ((UIP_TCP_BUF->flags & TCP_CTL) == (TCP_SYN | TCP_ACK))) ||
                    (((uip_connr->tcpstateflags & UIP_TS_MASK) == UIP_SYN_RCVD) &&
                            ((UIP_TCP_BUF->flags & TCP_CTL) == TCP_SYN))))
    {
        if((uip_len > 0 || ((UIP_TCP_BUF->flags & (TCP_SYN | TCP_FIN)) != 0)) &&
                (UIP_TCP_BUF->seqno[0] != uip_connr->rcv_nxt[0] ||
                        UIP_TCP_BUF->seqno[1] != uip_connr->rcv_nxt[1] ||
                        UIP_TCP_BUF->seqno[2] != uip_connr->rcv_nxt[2] ||
                        UIP_TCP_BUF->seqno[3] != uip_connr->rcv_nxt[3]))
        {

            if(UIP_TCP_BUF->flags & TCP_SYN)
            {
                goto tcp_send_synack;
            }
            goto tcp_send_ack;
        }
    }

    /* Next, check if the incoming segment acknowledges any outstanding
     data. If so, we update the sequence number, reset the length of
     the outstanding data, calculate RTT estimations, and reset the
     retransmission timer. */
    if((UIP_TCP_BUF->flags & TCP_ACK) && uip_outstanding(uip_connr))
    {
        uip_add32(uip_connr->snd_nxt, uip_connr->len);

        if(UIP_TCP_BUF->ackno[0] == uip_acc32[0] &&
                UIP_TCP_BUF->ackno[1] == uip_acc32[1] &&
                UIP_TCP_BUF->ackno[2] == uip_acc32[2] &&
                UIP_TCP_BUF->ackno[3] == uip_acc32[3])
        {
            /* Update sequence number. */
            uip_connr->snd_nxt[0] = uip_acc32[0];
            uip_connr->snd_nxt[1] = uip_acc32[1];
            uip_connr->snd_nxt[2] = uip_acc32[2];
            uip_connr->snd_nxt[3] = uip_acc32[3];

            /* Do RTT estimation, unless we have done retransmissions. */
            if(uip_connr->nrtx == 0)
            {
                signed char m;
                m = uip_connr->rto - uip_connr->timer;
                /* This is taken directly from VJs original code in his paper */
                m = m - (uip_connr->sa >> 3);
                uip_connr->sa += m;
                if(m < 0)
                {
                    m = -m;
                }
                m = m - (uip_connr->sv >> 2);
                uip_connr->sv += m;
                uip_connr->rto = (uip_connr->sa >> 3) + uip_connr->sv;

            }
            /* Set the acknowledged flag. */
            uip_flags = UIP_ACKDATA;
            /* Reset the retransmission timer. */
            uip_connr->timer = uip_connr->rto;

            /* Reset length of outstanding data. */
            uip_connr->len = 0;
        }

    }

    /* Do different things depending on in what state the connection is. */
    switch(uip_connr->tcpstateflags & UIP_TS_MASK)
    {
        /* CLOSED and LISTEN are not handled here. CLOSE_WAIT is not
         implemented, since we force the application to close when the
         peer sends a FIN (hence the application goes directly from
         ESTABLISHED to LAST_ACK). */
        case UIP_SYN_RCVD:
        /* In SYN_RCVD we have sent out a SYNACK in response to a SYN, and
         we are waiting for an ACK that acknowledges the data we sent
         out the last time. Therefore, we want to have the UIP_ACKDATA
         flag set. If so, we enter the ESTABLISHED state. */
        if(uip_flags & UIP_ACKDATA)
        {
            uip_connr->tcpstateflags = UIP_ESTABLISHED;
            uip_flags = UIP_CONNECTED;
            uip_connr->len = 0;
            if(uip_len > 0)
            {
                uip_flags |= UIP_NEWDATA;
                uip_add_rcv_nxt(uip_len);
            }
            uip_slen = 0;

            UIP_APPCALL_Dbg.uip_syn_rcvd++;
            UIP_APPCALL();
            goto appsend;
        }
        /* We need to retransmit the SYNACK */
        if((UIP_TCP_BUF->flags & TCP_CTL) == TCP_SYN)
        {
            goto tcp_send_synack;
        }
        goto drop;
#if UIP_ACTIVE_OPEN
        case UIP_SYN_SENT:
        /* In SYN_SENT, we wait for a SYNACK that is sent in response to
         our SYN. The rcv_nxt is set to sequence number in the SYNACK
         plus one, and we send an ACK. We move into the ESTABLISHED
         state. */
        if((uip_flags & UIP_ACKDATA) &&
                (UIP_TCP_BUF->flags & TCP_CTL) == (TCP_SYN | TCP_ACK))
        {

            /* Parse the TCP MSS option, if present. */
            if((UIP_TCP_BUF->tcpoffset & 0xf0) > 0x50)
            {
                for(c = 0; c < ((UIP_TCP_BUF->tcpoffset >> 4) - 5) << 2;)
                {
                    opt = uip_buf[UIP_IPTCPH_LEN + UIP_LLH_LEN + c];
                    if(opt == TCP_OPT_END)
                    {
                        /* End of options. */
                        break;
                    }
                    else if(opt == TCP_OPT_NOOP)
                    {
                        ++c;
                        /* NOP option. */
                    }
                    else if(opt == TCP_OPT_MSS &&
                            uip_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + 1 + c] == TCP_OPT_MSS_LEN)
                    {
                        /* An MSS option with the right option length. */
                        tmp16 = (uip_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + 2 + c] << 8) |
                        uip_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + 3 + c];
                        uip_connr->initialmss =
                        uip_connr->mss = tmp16 > UIP_TCP_MSS? UIP_TCP_MSS: tmp16;

                        /* And we are done processing options. */
                        break;
                    }
                    else
                    {
                        /* All other options have a length field, so that we easily
                         can skip past them. */
                        if(uip_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + 1 + c] == 0)
                        {
                            /* If the length field is zero, the options are malformed
                             and we don't process them further. */
                            break;
                        }
                        c += uip_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN + 1 + c];
                    }
                }
            }
            uip_connr->tcpstateflags = UIP_ESTABLISHED;
            uip_connr->rcv_nxt[0] = UIP_TCP_BUF->seqno[0];
            uip_connr->rcv_nxt[1] = UIP_TCP_BUF->seqno[1];
            uip_connr->rcv_nxt[2] = UIP_TCP_BUF->seqno[2];
            uip_connr->rcv_nxt[3] = UIP_TCP_BUF->seqno[3];
            uip_add_rcv_nxt(1);
            uip_flags = UIP_CONNECTED | UIP_NEWDATA;
            uip_connr->len = 0;
            uip_len = 0;
            uip_slen = 0;

            UIP_APPCALL_Dbg.uip_syn_sent++;
            UIP_APPCALL();
            goto appsend;
        }
        /* Inform the application that the connection failed */
        UIP_APPCALL_Dbg.conn_failure_uip_abort++;

        uip_flags = UIP_ABORT;
        UIP_APPCALL();
        /* The connection is closed after we send the RST */
        uip_conn->tcpstateflags = UIP_CLOSED;
        goto reset;
#endif /* UIP_ACTIVE_OPEN */

        case UIP_ESTABLISHED:
        /* In the ESTABLISHED state, we call upon the application to feed
         data into the uip_buf. If the UIP_ACKDATA flag is set, the
         application should put new data into the buffer, otherwise we are
         retransmitting an old segment, and the application should put that
         data into the buffer.

         If the incoming packet is a FIN, we should close the connection on
         this side as well, and we send out a FIN and enter the LAST_ACK
         state. We require that there is no outstanding data; otherwise the
         sequence numbers will be screwed up. */

        if(UIP_TCP_BUF->flags & TCP_FIN && !(uip_connr->tcpstateflags & UIP_STOPPED))
        {
            if(uip_outstanding(uip_connr))
            {
                goto drop;
            }
            uip_add_rcv_nxt(1 + uip_len);
            uip_flags |= UIP_CLOSE;
            if(uip_len > 0)
            {
                uip_flags |= UIP_NEWDATA;
            }
            UIP_APPCALL_Dbg.uip_newdata++;
            UIP_APPCALL();
            uip_connr->len = 1;
            uip_connr->tcpstateflags = UIP_LAST_ACK;
            uip_connr->nrtx = 0;
            tcp_send_finack:
            UIP_TCP_BUF->flags = TCP_FIN | TCP_ACK;
            goto tcp_send_nodata;
        }

        /* Check the URG flag. If this is set, the segment carries urgent
         data that we must pass to the application. */
        if((UIP_TCP_BUF->flags & TCP_URG) != 0)
        {
#if UIP_URGDATA > 0
            uip_urglen = (UIP_TCP_BUF->urgp[0] << 8) | UIP_TCP_BUF->urgp[1];
            if(uip_urglen > uip_len)
            {
                /* There is more urgent data in the next segment to come. */
                uip_urglen = uip_len;
            }
            uip_add_rcv_nxt(uip_urglen);
            uip_len -= uip_urglen;
            uip_urgdata = uip_appdata;
            uip_appdata += uip_urglen;
        }
        else
        {
            uip_urglen = 0;
#else /* UIP_URGDATA > 0 */
            uip_appdata = ((char *)uip_appdata) + ((UIP_TCP_BUF->urgp[0] << 8) | UIP_TCP_BUF->urgp[1]);
            uip_len -= (UIP_TCP_BUF->urgp[0] << 8) | UIP_TCP_BUF->urgp[1];
#endif /* UIP_URGDATA > 0 */
        }

        /* If uip_len > 0 we have TCP data in the packet, and we flag this
         by setting the UIP_NEWDATA flag and update the sequence number
         we acknowledge. If the application has stopped the dataflow
         using uip_stop(), we must not accept any data packets from the
         remote host. */
        if(uip_len > 0 && !(uip_connr->tcpstateflags & UIP_STOPPED))
        {
            uip_flags |= UIP_NEWDATA;
            uip_add_rcv_nxt(uip_len);
        }

        /* Check if the available buffer space advertised by the other end
         is smaller than the initial MSS for this connection. If so, we
         set the current MSS to the window size to ensure that the
         application does not send more data than the other end can
         handle.

         If the remote host advertises a zero window, we set the MSS to
         the initial MSS so that the application will send an entire MSS
         of data. This data will not be acknowledged by the receiver,
         and the application will retransmit it. This is called the
         "persistent timer" and uses the retransmission mechanim.
         */
        tmp16 = ((uint16_t)UIP_TCP_BUF->wnd[0] << 8) + (uint16_t)UIP_TCP_BUF->wnd[1];
        if(tmp16 > uip_connr->initialmss ||
                tmp16 == 0)
        {
            tmp16 = uip_connr->initialmss;
        }
        uip_connr->mss = tmp16;

        /* If this packet constitutes an ACK for outstanding data (flagged
         by the UIP_ACKDATA flag, we should call the application since it
         might want to send more data. If the incoming packet had data
         from the peer (as flagged by the UIP_NEWDATA flag), the
         application must also be notified.

         When the application is called, the global variable uip_len
         contains the length of the incoming data. The application can
         access the incoming data through the global pointer
         uip_appdata, which usually points UIP_IPTCPH_LEN + UIP_LLH_LEN
         bytes into the uip_buf array.

         If the application wishes to send any data, this data should be
         put into the uip_appdata and the length of the data should be
         put into uip_len. If the application don't have any data to
         send, uip_len must be set to 0. */
        if(uip_flags & (UIP_NEWDATA | UIP_ACKDATA))
        {
            uip_slen = 0;
            UIP_APPCALL_Dbg.uip_newdata_ackdata++;
            UIP_APPCALL();

            appsend:

            if(uip_flags & UIP_ABORT)
            {
                uip_slen = 0;
                uip_connr->tcpstateflags = UIP_CLOSED;
                UIP_TCP_BUF->flags = TCP_RST | TCP_ACK;
                goto tcp_send_nodata;
            }

            if(uip_flags & UIP_CLOSE)
            {
                uip_slen = 0;
                uip_connr->len = 1;
                uip_connr->tcpstateflags = UIP_FIN_WAIT_1;
                uip_connr->nrtx = 0;
                UIP_TCP_BUF->flags = TCP_FIN | TCP_ACK;
                goto tcp_send_nodata;
            }

            /* If uip_slen > 0, the application has data to be sent. */
            if(uip_slen > 0)
            {

                /* If the connection has acknowledged data, the contents of
                 the ->len variable should be discarded. */
                if((uip_flags & UIP_ACKDATA) != 0)
                {
                    uip_connr->len = 0;
                }

                /* If the ->len variable is non-zero the connection has
                 already data in transit and cannot send anymore right
                 now. */
                if(uip_connr->len == 0)
                {

                    /* The application cannot send more than what is allowed by
                     the mss (the minumum of the MSS and the available
                     window). */
                    if(uip_slen > uip_connr->mss)
                    {
                        uip_slen = uip_connr->mss;
                    }

                    /* Remember how much data we send out now so that we know
                     when everything has been acknowledged. */
                    uip_connr->len = uip_slen;
                }
                else
                {

                    /* If the application already had unacknowledged data, we
                     make sure that the application does not send (i.e.,
                     retransmit) out more than it previously sent out. */
                    uip_slen = uip_connr->len;
                }
            }
            uip_connr->nrtx = 0;
            apprexmit:
            uip_appdata = uip_sappdata;

            /* If the application has data to be sent, or if the incoming
             packet had new data in it, we must send out a packet. */
            if(uip_slen > 0 && uip_connr->len > 0)
            {
                /* Add the length of the IP and TCP headers. */
                uip_len = uip_connr->len + UIP_TCPIP_HLEN;
                /* We always set the ACK flag in response packets. */
                UIP_TCP_BUF->flags = TCP_ACK | TCP_PSH;
                /* Send the packet. */
                goto tcp_send_noopts;
            }
            /* If there is no data to send, just send out a pure ACK if
             there is newdata. */
            if(uip_flags & UIP_NEWDATA)
            {
                uip_len = UIP_TCPIP_HLEN;
                UIP_TCP_BUF->flags = TCP_ACK;
                goto tcp_send_noopts;
            }
        }
        goto drop;
        case UIP_LAST_ACK:
        /* We can close this connection if the peer has acknowledged our
         FIN. This is indicated by the UIP_ACKDATA flag. */
        if(uip_flags & UIP_ACKDATA)
        {
            uip_connr->tcpstateflags = UIP_CLOSED;
            uip_flags = UIP_CLOSE;
            UIP_APPCALL_Dbg.uip_last_ack_close++;
            UIP_APPCALL();
        }
        break;

        case UIP_FIN_WAIT_1:
        /* The application has closed the connection, but the remote host
         hasn't closed its end yet. Thus we do nothing but wait for a
         FIN from the other side. */
        if(uip_len > 0)
        {
            uip_add_rcv_nxt(uip_len);
        }
        if(UIP_TCP_BUF->flags & TCP_FIN)
        {
            if(uip_flags & UIP_ACKDATA)
            {
                uip_connr->tcpstateflags = UIP_TIME_WAIT;
                uip_connr->timer = 0;
                uip_connr->len = 0;
            }
            else
            {
                uip_connr->tcpstateflags = UIP_CLOSING;
            }
            uip_add_rcv_nxt(1);
            uip_flags = UIP_CLOSE;
            UIP_APPCALL_Dbg.uip_fin_wait_1_close++;
            UIP_APPCALL();
            goto tcp_send_ack;
        }
        else if(uip_flags & UIP_ACKDATA)
        {
            uip_connr->tcpstateflags = UIP_FIN_WAIT_2;
            uip_connr->len = 0;
            goto drop;
        }
        if(uip_len > 0)
        {
            goto tcp_send_ack;
        }
        goto drop;

        case UIP_FIN_WAIT_2:
        if(uip_len > 0)
        {
            uip_add_rcv_nxt(uip_len);
        }
        if(UIP_TCP_BUF->flags & TCP_FIN)
        {
            uip_connr->tcpstateflags = UIP_TIME_WAIT;
            uip_connr->timer = 0;
            uip_add_rcv_nxt(1);
            uip_flags = UIP_CLOSE;
            UIP_APPCALL_Dbg.uip_fin_wait_2_close++;
            UIP_APPCALL();
            goto tcp_send_ack;
        }
        if(uip_len > 0)
        {
            goto tcp_send_ack;
        }
        goto drop;

        case UIP_TIME_WAIT:
        goto tcp_send_ack;

        case UIP_CLOSING:
        if(uip_flags & UIP_ACKDATA)
        {
            uip_connr->tcpstateflags = UIP_TIME_WAIT;
            uip_connr->timer = 0;
        }
    }
    goto drop;

    /* We jump here when we are ready to send the packet, and just want
     to set the appropriate TCP sequence numbers in the TCP header. */
    tcp_send_ack:
    UIP_TCP_BUF->flags = TCP_ACK;

    tcp_send_nodata:
    uip_len = UIP_IPTCPH_LEN;

    tcp_send_noopts:
    UIP_TCP_BUF->tcpoffset = (UIP_TCPH_LEN / 4) << 4;

    /* We're done with the input processing. We are now ready to send a
     reply. Our job is to fill in all the fields of the TCP and IP
     headers before calculating the checksum and finally send the
     packet. */
    tcp_send:
    LOG_INFO("In tcp_send\n");

    UIP_TCP_BUF->ackno[0] = uip_connr->rcv_nxt[0];
    UIP_TCP_BUF->ackno[1] = uip_connr->rcv_nxt[1];
    UIP_TCP_BUF->ackno[2] = uip_connr->rcv_nxt[2];
    UIP_TCP_BUF->ackno[3] = uip_connr->rcv_nxt[3];

    UIP_TCP_BUF->seqno[0] = uip_connr->snd_nxt[0];
    UIP_TCP_BUF->seqno[1] = uip_connr->snd_nxt[1];
    UIP_TCP_BUF->seqno[2] = uip_connr->snd_nxt[2];
    UIP_TCP_BUF->seqno[3] = uip_connr->snd_nxt[3];

    UIP_IP_BUF->proto = UIP_PROTO_TCP;

    UIP_TCP_BUF->srcport = uip_connr->lport;
    UIP_TCP_BUF->destport = uip_connr->rport;

    uip_ipaddr_copy(&UIP_IP_BUF->destipaddr, &uip_connr->ripaddr);
    uip_ds6_select_src(&UIP_IP_BUF->srcipaddr,&UIP_IP_BUF->destipaddr);
    LOG_INFO("Sending TCP packet to");
    PRINT6ADDR(&UIP_IP_BUF->destipaddr);
    LOG_INFO("from");
    PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
    LOG_INFO("\n");

    if(uip_connr->tcpstateflags & UIP_STOPPED)
    {
        /* If the connection has issued uip_stop(), we advertise a zero
         window so that the remote host will stop sending data. */
        UIP_TCP_BUF->wnd[0] = UIP_TCP_BUF->wnd[1] = 0;
    }
    else
    {
        UIP_TCP_BUF->wnd[0] = ((UIP_RECEIVE_WINDOW) >> 8);
        UIP_TCP_BUF->wnd[1] = ((UIP_RECEIVE_WINDOW) & 0xff);
    }

    tcp_send_noconn:
    UIP_IP_BUF->ttl = uip_ds6_if.cur_hop_limit;
    UIP_IP_BUF->len0 = ((uip_len - UIP_IPH_LEN) >> 8);
    UIP_IP_BUF->len1 = ((uip_len - UIP_IPH_LEN) & 0xff);

    UIP_TCP_BUF->urgp[0] = UIP_TCP_BUF->urgp[1] = 0;

    /* Calculate TCP checksum. */
    UIP_TCP_BUF->tcpchksum = 0;
    UIP_TCP_BUF->tcpchksum = ~(uip_tcpchksum());
    UIP_STAT(++uip_stat.tcp.sent);

#endif /* UIP_TCP */
#if UIP_UDP
    ip_send_nolen:
#endif
    UIP_IP_BUF->vtc = 0x60;
    UIP_IP_BUF->tcflow = 0x00;
    UIP_IP_BUF->flow = 0x0000;
    send: LOG_INFO2("Sending packet with length %d (%d)\n", uip_len,
            (UIP_IP_BUF->len0 << 8) | UIP_IP_BUF->len1);

    UIP_STAT(++uip_stat.ip.sent);
    /* Return and let the caller do the actual transmission. */
    uip_flags = 0;
    return;

    drop:
    uip_len= 0;
    uip_ext_len = 0;
    uip_ext_bitmap = 0;
    uip_flags = 0;
    return;
}
/*---------------------------------------------------------------------------*/
u16_t uip_htons(u16_t val)
{
    return UIP_HTONS(val);
}

u32_t uip_htonl(u32_t val)
{
    return UIP_HTONL(val);
}
/*---------------------------------------------------------------------------*/

void uip_send(const void *data, int len)
{
    int copylen;
#define MIN(a,b) ((a) < (b)? (a): (b))
    copylen =
            MIN(len,
                BYTE_UNITS(UIP_BUFSIZE) - BYTE_UNITS(UIP_LLH_LEN) - BYTE_UNITS(UIP_TCPIP_HLEN) - BYTE_UNITS((int)((char *)uip_sappdata - (char *)&uip_buf[UIP_LLH_LEN + UIP_TCPIP_HLEN]))); //Everything to Byte units (len is already in Byte units)
    if (copylen > 0)
    {
        uip_slen = copylen;
        if (data != uip_sappdata)
        {
            pltfrm_byte_memcpy((void* )uip_sappdata, 0, (void* )(data), 0,
                               uip_slen);
        }
    }
}

int uip_null_hbho_hdr_ext(int action, void* data)
{
    return 0;
}

/*---------------------------------------------------------------------------*/
/** @} */

//link layer address of a received packet or packet to send
//uip_lladdr_t uip_src_lladdr;
//uip_lladdr_t uip_dst_lladdr;
#endif /* UIP_CONF_IPV6 */
