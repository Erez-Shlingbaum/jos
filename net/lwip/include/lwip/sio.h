/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 */

/*
 * This is the interface to the platform specific serial IO module
 * It needs to be implemented by those platforms which need SLIP or PPP
 */

#ifndef __SIO_H__
#define __SIO_H__

#include "lwip/arch.h"

#ifdef __cplusplus
extern "C" {
#endif

/* If you want to define sio_fd_t elsewhere or differently,
   define this in your cc.h file. */
#ifndef __sio_fd_t_defined
typedef void *sio_fd_t;
#endif

/* The following functions can be defined to something else in your cc.h file
   or be implemented in your custom sio.c file. */

#ifndef sio_open
sio_fd_t sio_open(u8_t);
#endif

#ifndef sio_send
void sio_send(u8_t, sio_fd_t);
#endif

#ifndef sio_recv
u8_t sio_recv(sio_fd_t);
#endif

#ifndef sio_read
u32_t sio_read(sio_fd_t, u8_t *, u32_t);
#endif

#ifndef sio_write
u32_t sio_write(sio_fd_t, u8_t *, u32_t);
#endif

#ifndef sio_read_abort
void sio_read_abort(sio_fd_t);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __SIO_H__ */
