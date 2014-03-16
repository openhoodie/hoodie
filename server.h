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

/* this include file is only for use by the xatmi implementation functions */

#include <assert.h>
#include <xatmi.h>
#include "byteorder.h"
#include "queue.h"
#include <sys/time.h>
#include <search.h>
//#include <stdlib.h>


#define TPERRNO 0x3	/* trans failed.  tperrno returned.  goes with TPSUCCESS & TPFAIL */



/* message types.  start at 0.  no gaps.  CO-ORDINATE WITH COMMAND TABLE in client.c. */
#define TPTCALL	0x0	/* message type: call from client */
#define TPTGRPLY 0x1	/* message type: request for reply from client */
#define TPTADVT 0x2
#define TPTUADV 0x3
#define TPTWORK 0x4
#define TPTRETN 0x5


/*******************************************************************************************/



struct tpclient {
	QUEUE global;				/* global client queue */
	int fd;					/* our communication socket */

	/* used, input and handler are helpers for svr_io_wait and Iowait */
	int used;				/* number of bytes of input to read */
	char *input;				/* buffer from tpalloc */
	void (*handler)(struct tpclient*,char*);/* function to call when buffer full */
	char *handler_name;

	void *adverts;				/* tree of services we advertise */
	QUEUE requests;				/* unprocessed transaction */
	QUEUE replies;				/* completed transactions */
	struct tptransaction *wait4;		/* transaction we are servicing (worker) or waiting for (client) */
};

extern QUEUE tpclients;


struct tpclient *new_client(int sock);
void del_client(struct tpclient *);


/*******************************************************************************************/


/*
 * queue of advertised services.
 * the wait queue contains either clients, which are waiting to do work for this
 * service, or transactions, which need a client to serve them.
 */
struct tpservice {
	char svcname[XATMI_SERVICE_NAME_LENGTH];
	QUEUE waitq;				/* contains transactions or advertisements */
	int wait_contains_transactions;		/* or advertisements */
	unsigned long tmsec, ttrans;		/* msecs spent in service, trans served */
};

struct tpservice *fnd_service(char *svcname);
struct tpservice *new_service(char *svcname);
void del_service(struct tpservice *svc);


/*******************************************************************************************/


/* service advertisement table */
struct tpadvertisement {
	struct tpservice *svc;			/* service on offer */
	struct tpclient *client;		/* client making offer */
	QUEUE svc_waitq;			/* service wait queue */
	unsigned long opaque;			/* func, passed by client in tpadvertise */
	unsigned ttrans;			/* total trans served */
	unsigned long tmsec;			/* total msecs spent in service */
};

struct tpadvertisement *fnd_advertisement(struct tpclient*, char *svcname);
struct tpadvertisement *new_advertisement(struct tpclient*, char *svcname, long opaque);
void del_advertisement(struct tpadvertisement *adv);


/*******************************************************************************************/



/* each transaction */
struct tptransaction {
	struct tpclient *client;		/* submiting client */
	struct tpservice *svc;			/* which service we need */
	struct timeval arrival;			/* when we were submitted */
	QUEUE global;				/* global list of transactions */
	QUEUE clientq;				/* client's transaction queue */
	QUEUE svc_waitq;			/* service wait queue */
	struct tpadvertisement *executor;	/* client now serving us */
	short cd;				/* unique for all current transactions; not 0 */
	char *data;				/* data or reply */
	int rval;				/* filled in by service handler */
	long rcode;				/* filled in by service handler */
	long flags;
};


struct tptransaction *new_transaction(struct tpclient *c, long flags, struct tpservice *svc, long len);
void del_transaction(struct tptransaction *t);


/*******************************************************************************************/

void svriowait(struct tpclient *, char *data, void (*func)(struct tpclient *, char *), char *func_name);
void svrwrite(struct tpclient *, void *, int len);
void svrwriteb(struct tpclient *, unsigned char);
void svrwrites(struct tpclient *, unsigned short);
void svrwritel(struct tpclient *, unsigned long);


/*******************************************************************************************/


void svr_next_command(struct tpclient *c);

void svracall(struct tpclient *c, char *buf);
void svradvertise(struct tpclient *c, char *buf);
void svrgetrply(struct tpclient *c, char *buf);
void svrreturn(struct tpclient *c, char *buf);
void svrservice(struct tpclient *c, char *buf);
void svrunadvertise(struct tpclient *c, char *buf);


/*******************************************************************************************/


void send_to_server(struct tpadvertisement *a, struct tptransaction *t);
void snd_reply(struct tptransaction *t);


/*******************************************************************************************/


void debug(int level, char *fmt, ...);
void fatal(char *fmt, ...);
void error(char *fmt, ...);

#endif /* __XATMILIB_H__ */
