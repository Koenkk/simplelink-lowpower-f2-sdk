/**
 * \addtogroup uip
 * @{
 */

/**
 * \defgroup tcpip The Contiki/uIP interface
 * @{
 *
 * TCP/IP support in Contiki is implemented using the uIP TCP/IP
 * stack. For sending and receiving data, Contiki uses the functions
 * provided by the uIP module, but Contiki adds a set of functions for
 * connection management. The connection management functions make
 * sure that the uIP TCP/IP connections are connected to the correct
 * process.
 *
 * Contiki also includes an optional protosocket library that provides
 * an API similar to the BSD socket API.
 *
 * \sa \ref uip "The uIP TCP/IP stack"
 * \sa \ref psock "Protosockets library"
 *
 */

/**
 * \file
 *          Header for the Contiki/uIP interface.
 * \author  Adam Dunkels <adam@sics.se>
 * \author  Mathilde Durvy <mdurvy@cisco.com> (IPv6 related code)
 * \author  Julien Abeille <jabeille@cisco.com> (IPv6 related code)
 */

/*
 * Copyright (c) 2004, Swedish Institute of Computer Science.
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 * $Id: tcpip.h,v 1.17 2010/10/19 18:29:04 adamdunkels Exp $
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
#ifndef __TCPIP_H__
#define __TCPIP_H__

//#include "contiki.h"
#include "uip-conf.h"
#include "sys/process.h"

#define UIP_TX_STATUS_PACKET_TOO_BIG            0
#define UIP_TX_STATUS_DESTINATION_UNSPECIFIED   1
#define UIP_TX_STATUS_DESTINATION_IS_OWN_ADDR   2
#define UIP_TX_STATUS_NO_NBR_FOR_NEXT_HOP       3
#define UIP_TX_STATUS_NO_ROUTE_TO_HOST          4
#define UIP_TX_STATUS_INVALID_PATH              5
#define UIP_TX_STATUS_CANNOT_INSERT_SRH         6
#define UIP_TX_STATUS_NBR_STATE_INCOMPLETE      7
#define UIP_TX_STATUS_SCHEDULED                 8
#define UIP_TX_STATUS_SCHEDULED_ERROR           9

#ifdef UIP_TX_STATUS_NOTIFICATION_FUNC   
#define UIP_TX_STATUS_NOTIFICATION(_status) UIP_TX_STATUS_NOTIFICATION_FUNC(_status)
void UIP_TX_STATUS_NOTIFICATION(int status);
#else
#define UIP_TX_STATUS_NOTIFICATION(_status)
#endif 

extern struct etimer periodic; //TCP periodic timer

struct uip_conn;

extern process_obj_t tcpip_process_obj;

#ifndef TCP_CONF_PERIODIC_TIMER
#define TCP_PERIODIC_TIMER_TICKS        (CLOCK_SECOND/2)
#else
#define TCP_PERIODIC_TIMER_TICKS        ((TCP_CONF_PERIODIC_TIMER/1000) * CLOCK_SECOND)
#endif

struct tcpip_uipstate
{
    process_obj_t *p;
    void *state;
};

#define UIP_APPCALL tcpip_uipcall
#define UIP_UDP_APPCALL tcpip_uipcall
#define UIP_ICMP6_APPCALL tcpip_icmp6_call

/*#define UIP_APPSTATE_SIZE sizeof(struct tcpip_uipstate)*/

typedef struct tcpip_uipstate uip_udp_appstate_t;
typedef struct tcpip_uipstate uip_tcp_appstate_t;
typedef struct tcpip_uipstate uip_icmp6_appstate_t;
#include "net/uip.h"
void tcpip_uipcall(void);

#ifdef UIP_FALLBACK_INTERFACE
extern const struct uip_fallback_interface UIP_FALLBACK_INTERFACE;
#endif

typedef process_post_func_t tcpip_process_post_function_t;

void tcpip_init(uip_lladdr_t* my_mac, tcpip_process_post_function_t func);

void eventhandler(process_event_t ev, process_data_t data);

#define tcpip_process_handler(event,data)	eventhandler(event,data)

enum
{
    TCP_POLL, UDP_POLL, PACKET_INPUT
};

#define UIP_APP_EVENT   0xAE //events to be processed by application

/**
 * \name TCP functions
 * @{
 */

#if UIP_CONF_TCP
/**
 * Attach a TCP connection to the current process
 *
 * This function attaches the current process to a TCP
 * connection. Each TCP connection must be attached to a process in
 * order for the process to be able to receive and send
 * data. Additionally, this function can add a pointer with connection
 * state to the connection.
 *
 * \param conn A pointer to the TCP connection.
 *
 * \param appstate An opaque pointer that will be passed to the
 * process whenever an event occurs on the connection.
 *
 */
CCIF void tcp_attach(struct uip_conn *conn,
        void *appstate, process_obj_t *process_obj);
#define tcp_markconn(conn, appstate, p) tcp_attach(conn, appstate, p)

/**
 * Open a TCP port.
 *
 * This function opens a TCP port for listening. When a TCP connection
 * request occurs for the port, the process will be sent a tcpip_event
 * with the new connection request.
 *
 * \note Port numbers must always be given in network byte order. The
 * functions UIP_HTONS() and uip_htons() can be used to convert port numbers
 * from host byte order to network byte order.
 *
 * \param port The port number in network byte order.
 *
 */
CCIF void tcp_listen(uint16_t port, process_obj_t *process_obj);

/**
 * Close a listening TCP port.
 *
 * This function closes a listening TCP port.
 *
 * \note Port numbers must always be given in network byte order. The
 * functions UIP_HTONS() and uip_htons() can be used to convert port numbers
 * from host byte order to network byte order.
 *
 * \param port The port number in network byte order.
 *
 */
CCIF void tcp_unlisten(uint16_t port, process_obj_t *process_obj);

/**
 * Open a TCP connection to the specified IP address and port.
 *
 * This function opens a TCP connection to the specified port at the
 * host specified with an IP address. Additionally, an opaque pointer
 * can be attached to the connection. This pointer will be sent
 * together with uIP events to the process.
 *
 * \note The port number must be provided in network byte order so a
 * conversion with UIP_HTONS() usually is necessary.
 *
 * \note This function will only create the connection. The connection
 * is not opened directly. uIP will try to open the connection the
 * next time the uIP stack is scheduled by Contiki.
 *
 * \param ripaddr Pointer to the IP address of the remote host.
 * \param port Port number in network byte order.
 * \param appstate Pointer to application defined data.
 *
 * \return A pointer to the newly created connection, or NULL if
 * memory could not be allocated for the connection.
 *
 */
CCIF struct uip_conn *tcp_connect(uip_ipaddr_t *ripaddr, uint16_t port,
        void *appstate, process_obj_t *process_obj);

/**
 * Cause a specified TCP connection to be polled.
 *
 * This function causes uIP to poll the specified TCP connection. The
 * function is used when the application has data that is to be sent
 * immediately and do not wish to wait for the periodic uIP polling
 * mechanism.
 *
 * \param conn A pointer to the TCP connection that should be polled.
 *
 */
void tcpip_poll_tcp(struct uip_conn *conn);

#endif

/** @} */

/**
 * \name UDP functions
 * @{
 */

struct uip_udp_conn;
/**
 * Attach the current process to a UDP connection
 *
 * This function attaches the current process to a UDP
 * connection. Each UDP connection must have a process attached to it
 * in order for the process to be able to receive and send data over
 * the connection. Additionally, this function can add a pointer with
 * connection state to the connection.
 *
 * \param conn A pointer to the UDP connection.
 *
 * \param appstate An opaque pointer that will be passed to the
 * process whenever an event occurs on the connection.
 *
 */
void udp_attach(struct uip_udp_conn *conn, void *appstate,
                process_obj_t *process_obj);
#define udp_markconn(conn, appstate, p) udp_attach(conn, appstate, p)

/**
 * Create a new UDP connection.
 *
 * This function creates a new UDP connection with the specified
 * remote endpoint.
 *
 * \note The port number must be provided in network byte order so a
 * conversion with UIP_HTONS() usually is necessary.
 *
 * \sa udp_bind()
 *
 * \param ripaddr Pointer to the IP address of the remote host.
 * \param port Port number in network byte order.
 * \param appstate Pointer to application defined data.
 *
 * \return A pointer to the newly created connection, or NULL if
 * memory could not be allocated for the connection.
 */
CCIF struct uip_udp_conn *udp_new(const uip_ipaddr_t *ripaddr, u16_t port,
                                  void *appstate, process_obj_t *process_obj);
/**
 * Create a new UDP broadcast connection.
 *
 * This function creates a new (link-local) broadcast UDP connection
 * to a specified port.
 *
 * \param port Port number in network byte order.
 * \param appstate Pointer to application defined data.
 *
 * \return A pointer to the newly created connection, or NULL if
 * memory could not be allocated for the connection.
 */
struct uip_udp_conn *udp_broadcast_new(u16_t port, void *appstate,
                                       process_obj_t *process_obj);

/**
 * Bind a UDP connection to a local port.
 *
 * This function binds a UDP connection to a specified local port.
 *
 * When a connection is created with udp_new(), it gets a local port
 * number assigned automatically. If the application needs to bind the
 * connection to a specified local port, this function should be used.
 *
 * \note The port number must be provided in network byte order so a
 * conversion with UIP_HTONS() usually is necessary.
 *
 * \param conn A pointer to the UDP connection that is to be bound.
 * \param port The port number in network byte order to which to bind
 * the connection.
 */
#define udp_bind(conn, port) uip_udp_bind(conn, port)

/**
 * Cause a specified UDP connection to be polled.
 *
 * This function causes uIP to poll the specified UDP connection. The
 * function is used when the application has data that is to be sent
 * immediately and do not wish to wait for the periodic uIP polling
 * mechanism.
 *
 * \param conn A pointer to the UDP connection that should be polled.
 *
 */
CCIF void tcpip_poll_udp(struct uip_udp_conn *conn);

/** @} */

/**
 * \name ICMPv6 functions
 * @{
 */

#if UIP_CONF_ICMP6

/**
 * The ICMP6 event.
 *
 * This event is posted to a process whenever a uIP ICMP event has occurred.
 */
CCIF extern process_event_t tcpip_icmp6_event;

/**
 * \brief register an ICMPv6 callback
 * \return 0 if success, 1 if failure (one application already registered)
 *
 * This function just registers a process to be polled when
 * an ICMPv6 message is received.
 * If no application registers, some ICMPv6 packets will be
 * processed by the "kernel" as usual (NS, NA, RS, RA, Echo request),
 * others will be dropped.
 * If an application registers here, it will be polled with a
 * process_post_synch every time an ICMPv6 packet is received.
 */
u16_t icmp6_new(void *appstate);

/**
 * This function is called at reception of an ICMPv6 packet
 * If an application registered as an ICMPv6 listener (with
 * icmp6_new), it will be called through a process_post_synch()
 */
void tcpip_icmp6_call(u16_t type);
#endif /*UIP_CONF_ICMP6*/

/** @} */
/**
 * The uIP event.
 *
 * This event is posted to a process whenever a uIP event has occurred.
 */

CCIF extern process_event_t tcpip_event;
/**
 * \name TCP/IP packet processing
 * @{
 */

/**
 * \brief      Deliver an incoming packet to the TCP/IP stack
 *
 *             This function is called by network device drivers to
 *             deliver an incoming packet to the TCP/IP stack. The
 *             incoming packet must be present in the uip_buf buffer,
 *             and the length of the packet must be in the global
 *             uip_len variable.
 */
CCIF void tcpip_input(process_data_t data);

/**
 * \brief Output packet to layer 2
 * The eventual parameter is the MAC address of the destination.
 */
#if UIP_CONF_IPV6
u16_t tcpip_output(uip_lladdr_t *);
void tcpip_set_outputfunc(u16_t (*f)(uip_lladdr_t *));
#else
u16_t tcpip_output(void);
void tcpip_set_outputfunc(u16_t (* f)(void));
#endif

/**
 * \brief This function does address resolution and then calls tcpip_output
 */
#if UIP_CONF_IPV6
void tcpip_ipv6_output(void);
#endif

/**
 * \brief Is forwarding generally enabled?
 */
extern unsigned char tcpip_do_forwarding;

/*
 * Are we at the moment forwarding the contents of uip_buf[]?
 */
extern unsigned char tcpip_is_forwarding;

#define tcpip_set_forwarding(forwarding) tcpip_do_forwarding = (forwarding)

//Returns 1 if the tcpip module has been initialized. 0 in other case
int tcpip_isInitialized();

//Stops the timers and frees memory if needed
void tcpip_clean();

/** @} */

#endif /* __TCPIP_H__ */

/** @} */
/** @} */
