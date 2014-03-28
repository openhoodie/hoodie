// XXX snd_reply maybe shouldn't be here
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
#include "server.h"

#define TRANSACTION(q) OFFSET(q, struct tptransaction, clientq)

static void senderrno(struct tpclient *c, int errno)
{
	debug(1, "returning error %d to %d", errno, c->fd);
	svrwriteb(c, TPERRNO);
	svrwrites(c, errno);
	svr_next_command(c);
}


void snd_reply(struct tptransaction *t)
{
	assert(t && t->svc && t->client);
	debug(1, "sending reply for %d (%.*s) to %d", t->cd, XATMI_SERVICE_NAME_LENGTH, t->svc->svcname, t->client->fd);
	int len = tptypes(t->data, 0, 0);
	svrwriteb(t->client, t->rval);
	svrwrites(t->client, t->cd);
	svrwritel(t->client, t->rcode);
	svrwritel(t->client, len);
	svrwrite(t->client, t->data, len);
	svr_next_command(t->client);
	t->client->wait4 = 0;
	del_transaction(t);
	return;
}

void svrgetrply(struct tpclient *c, char *buf)
{
	assert(c && buf);
	short cd = toshort(buf);
	long flags = tolong(buf+IOSHORT);
	debug(1, "svrgetrply(%d, %d)", c->fd, cd);
	tpfree(buf);

	/* check if reply available */
	QUEUE *q = QFIRST(c->replies);
	if (~flags & TPGETANY)
		for ( ; q != &c->replies; q = QNEXT(q))
			if (TRANSACTION(q)->cd == cd)
				break;
	if (q != &c->replies)
	{
		debug(1, "%d: returning extant reply for %d", c->fd, TRANSACTION(q)->cd);
		snd_reply(TRANSACTION(q));
	}
	else
	{
		/* ensure cd valid */
		extern struct tptransaction global_any_transaction;
		if (flags & TPGETANY)
			q = &global_any_transaction.clientq;
		else
			for (q = QFIRST(c->requests); q != &c->requests; q = QNEXT(q))
				if (TRANSACTION(q)->cd == cd)
					break;
		if (q == &c->requests)
			senderrno(c, TPEBADDESC);

		else if (flags & TPNOBLOCK)
			senderrno(c, TPEBLOCK);
		else
		{
			/* leaving them hanging... */
			debug(2, "%d: no reply yet.  waiting...", c->fd);
			c->wait4 = TRANSACTION(q);
		}
	}
}
