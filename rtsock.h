/*
 * rtsock.h - version 1.0
 *
 * Written by Robert Kavaler, 1998-2002
 *
 * Copyright (C) 1998-2002, Innomedia, Inc.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. 
 *
 * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials
 * provided with the distribution. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS
 * AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RTSOCK_MAJOR
#define RTSOCK_MAJOR	121
#endif

#define RTSOCK_NDEVS 	6

/*
 * Ioctl definitions
 */

#define RTSOCK_IOC_MAGIC  'r'

#define RTSOCK_IOCREDIRECTFD	_IOW( RTSOCK_IOC_MAGIC, 2, int)
#define RTSOCK_IOCRESETFD	_IOW( RTSOCK_IOC_MAGIC, 3, int)
#define RTSOCK_IOCGETSOCKADDR	_IOWR(RTSOCK_IOC_MAGIC, 4, int)
#define	RTSOCK_IOCINTERFACE	_IOWR(RTSOCK_IOC_MAGIC, 5, int)

#ifdef __KERNEL__

struct RtSock  *rtsock_create(int minor, 
			int fromRTmax_count, 
			int toRTmax_count, 
			int skbLength);

void 		rtsock_register_callback(struct RtSock *r, 
			int (*callback)(void *, struct sock *, unsigned long),
			void *callbackData);

void		rtsock_destroy(struct RtSock *r);
struct sk_buff *rtsock_alloc_skb(struct RtSock *r);
struct sk_buff *rtsock_dequeue_skb(struct RtSock *r);
void 		rtsock_enqueue_skb(struct RtSock *r, struct sk_buff *skb);
void 		rtsock_free_skb(struct RtSock *r, struct sk_buff *skb);

#ifdef LINUX_VERSION_CODE
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)

#define	rtsock_reference_sock(r,s)	atomic_add(1, &(s)->wmem_alloc)
#define	rtsock_dereference_sock(r,s)	atomic_sub(1, &(s)->wmem_alloc)

#else

#define	rtsock_reference_sock(r,s)	sock_hold(s)

void rtsock_dereference_sock(struct RtSock *r, struct sock *sk);

#endif
#endif

#endif

