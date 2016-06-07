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
/* echo.c - Networking echo server */


/*
 * Note that both the nano and microkernel images in this example
 * have a dummy fiber/task that does nothing. This is just here to
 * simulate a multi application scenario.
 */

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT           printf
#else
#include <misc/printk.h>
#define PRINT           printk
#endif

#include <net/net_core.h>
#include <net/net_socket.h>

/* The peer is the client in our case. Just invent a mac
 * address for it because lower parts of the stack cannot set it
 * in this test as we do not have any radios.
 */
static uint8_t peer_mac[] = { 0x15, 0x0a, 0xbe, 0xef, 0xf0, 0x0d };

/* This is my mac address
 */
static uint8_t my_mac[] = { 0x0a, 0xbe, 0xef, 0x15, 0xf0, 0x0d };

#ifdef CONFIG_NETWORKING_WITH_IPV6
/* The 2001:db8::/32 is the private address space for documentation RFC 3849 */
#define MY_IPADDR { { { 0x20,0x01,0x0d,0xb8,0,0,0,0,0,0,0,0,0,0,0,0x2 } } }
#else
/* The 192.0.2.0/24 is the private address space for documentation RFC 5737 */
#define MY_IPADDR { { { 192,0,2,2 } } }
#endif
#define MY_PORT 4242

static inline void init_server()
{
	PRINT("%s: run echo server\n", __FUNCTION__);

	net_set_mac(my_mac, sizeof(my_mac));

#ifdef CONFIG_NETWORKING_WITH_IPV4
	{
		uip_ipaddr_t addr;
		uip_ipaddr(&addr, 192,0,2,2);
		uip_sethostaddr(&addr);
	}
#endif
}

static inline void reverse(unsigned char *buf, int len)
{
	int i, last = len - 1;

	for (i = 0; i < len / 2; i++) {
		unsigned char tmp = buf[i];
		buf[i] = buf[last - i];
		buf[last - i] = tmp;
	}
}

static inline void receive_and_reply(const char *name, struct net_context *ctx)
{
	struct net_buf *buf;

	buf = net_receive(ctx);
	if (buf) {
		PRINT("%s: received %d bytes\n", name, uip_appdatalen(buf));

		/* In this test we reverse the received bytes.
		 * We could just pass the data back as is but
		 * this way it is possible to see how the app
		 * can manipulate the received data.
		 */
		reverse(uip_appdata(buf), uip_appdatalen(buf));

		/* Set the mac address of the peer in net_buf because
		 * there is no radio layer involved in this test app.
		 * Normally there is no need to do this.
		 */
		memcpy(&buf->src, &peer_mac, sizeof(buf->src));

		if (net_reply(ctx, buf)) {
			net_buf_put(buf);
		}
	}
}

static inline struct net_context *get_context(void)
{
	struct net_context *ctx;
	static struct net_addr any_addr;
	static struct net_addr my_addr;

#ifdef CONFIG_NETWORKING_WITH_IPV6
	static const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
	static struct in6_addr in6addr_my = MY_IPADDR;

	any_addr.in6_addr = in6addr_any;
	any_addr.family = AF_INET6;

	my_addr.in6_addr = in6addr_my;
	my_addr.family = AF_INET6;
#else
	static const struct in_addr in4addr_any = { { { 0 } } };
	static struct in_addr in4addr_my = MY_IPADDR;

	any_addr.in_addr = in4addr_any;
	any_addr.family = AF_INET;

	my_addr.in_addr = in4addr_my;
	my_addr.family = AF_INET;
#endif

	ctx = net_context_get(IPPROTO_UDP,
			      &any_addr, 0,
			      &my_addr, MY_PORT);
	if (!ctx) {
		PRINT("%s: Cannot get network context\n", __FUNCTION__);
		return NULL;
	}

	return ctx;
}

#ifdef CONFIG_MICROKERNEL

#include <zephyr.h>

/* specify delay between turns (in ms); compute equivalent in ticks */

#define SLEEPTIME  500
#define SLEEPTICKS (SLEEPTIME * sys_clock_ticks_per_sec / 1000)

/* specify delay between greetings (in ms); compute equivalent in ticks */

#define SLEEPTIME  500
#define SLEEPTICKS (SLEEPTIME * sys_clock_ticks_per_sec / 1000)

/*
*
* \param taskname    task identification string
* \param mySem       task's own semaphore
* \param otherSem    other task's semaphore
*
*/
void helloLoop(const char *taskname, ksem_t mySem, ksem_t otherSem)
{
	static struct net_context *ctx;

	net_init();

	if (!ctx) {
		ctx = get_context();
	}

	while (1) {
		task_sem_take_wait(mySem);

		receive_and_reply(taskname, ctx);

		/* wait a while, then let other task have a turn */
		task_sleep(SLEEPTICKS);
		task_sem_give(otherSem);
	}
}

void taskA(void)
{
	init_server();

	/* taskA gives its own semaphore, allowing it to say hello right away */
	task_sem_give(TASKASEM);

	/* invoke routine that allows task to ping-pong hello messages with taskB */
	helloLoop(__FUNCTION__, TASKASEM, TASKBSEM);
}

void taskB(void)
{
	/* invoke routine that allows task to ping-pong hello messages with taskA */
	helloLoop(__FUNCTION__, TASKBSEM, TASKASEM);
}

#else /*  CONFIG_NANOKERNEL */

/*
 * Nanokernel version of hello world demo has a task and a fiber that utilize
 * semaphores and timers to take turns printing a greeting message at
 * a controlled rate.
 */

#include <nanokernel.h>

/* specify delay between greetings (in ms); compute equivalent in ticks */

#define SLEEPTIME  500
#define SLEEPTICKS (SLEEPTIME * sys_clock_ticks_per_sec / 1000)

#define STACKSIZE 2000

char fiberStack[STACKSIZE];

struct nano_sem nanoSemTask;
struct nano_sem nanoSemFiber;

void fiberEntry(void)
{
	struct net_context *ctx;

	struct nano_timer timer;
	uint32_t data[2] = {0, 0};

	ctx = get_context();
	if (!ctx) {
		PRINT("%s: Cannot get network context\n", __FUNCTION__);
		return;
	}

	nano_sem_init (&nanoSemFiber);
	nano_timer_init (&timer, data);

	while (1) {
		/* wait for task to let us have a turn */
		nano_fiber_sem_take_wait (&nanoSemFiber);

		receive_and_reply(__FUNCTION__, ctx);

		/* wait a while, then let task have a turn */
		nano_fiber_timer_start (&timer, SLEEPTICKS);
		nano_fiber_timer_wait (&timer);
		nano_fiber_sem_give (&nanoSemTask);
	}
}

void main(void)
{
	struct nano_timer timer;
	uint32_t data[2] = {0, 0};

	net_init();

	init_server();

	task_fiber_start (&fiberStack[0], STACKSIZE,
			(nano_fiber_entry_t) fiberEntry, 0, 0, 7, 0);

	nano_sem_init(&nanoSemTask);
	nano_timer_init(&timer, data);

	while (1) {
		/* wait a while, then let fiber have a turn */
		nano_task_timer_start(&timer, SLEEPTICKS);
		nano_task_timer_wait(&timer);
		nano_task_sem_give(&nanoSemFiber);

		/* now wait for fiber to let us have a turn */
		nano_task_sem_take_wait(&nanoSemTask);
	}
}

#endif /* CONFIG_MICROKERNEL ||  CONFIG_NANOKERNEL */
