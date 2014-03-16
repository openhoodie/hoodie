/* only include this file in xatmi implementation functions */

/* 
** Copyright (c) 2007, DNA Pty Ltd and contributors
** 
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
** 
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
** 
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
** THE SOFTWARE.
*/

#include "byteorder.h"
#ifndef __XATMILIB_H__
#define __XATMILIB_H__

#include <xatmi.h>
#include <tx.h>
#include "byteorder.h"


#define TPERRNO 0x3	/* trans failed.  tperrno returned.  goes with TPSUCCESS & TPFAIL */

/* message types.  start at 0.  no gaps.  CO-ORDINATE WITH COMMAND TABLE in client.c. */
#define TPTCALL	0x0	/* message type: call from client */
#define TPTGRPLY 0x1	/* message type: request for reply from client */
#define TPTADVT 0x2
#define TPTUADV 0x3
#define TPTWORK 0x4
#define TPTRETN 0x5

#define MAGIC 0xfee1900d
#define TYPELEN 8
#define SUBTYPELEN 16
#define IS_MAGIC(p) (p && ((long*)p)[-1] == MAGIC)
#define ALLOC_UNITS(s) (((s) + sizeof(struct tpmemory) - 1) / sizeof(struct tpmemory) + 2)

struct tpmemory {
	char *type, *subtype;
	long len, magic;
};


#endif /* __XATMILIB_H__ */
