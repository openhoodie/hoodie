// XXX lots of stuff about advertisement
// XXX send_to_server maybe shouldn't be here
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
#include <sys/time.h>
#include <search.h>


/* first transaction on an advertised service's queue */
#define FIRSTTRAN(a) OFFSET(QFIRST((a)->svc->waitq), struct tptransaction, svc_waitq)

/* returns advertisement for earlest transaction we can service */
#define ADVERT(p) (*(struct tpadvertisement **)p)
static struct tpadvertisement *earliest;
static void find_earliest(const void *p, VISIT which, int depth)
{
	if (which == preorder || which == leaf)
		if (ADVERT(p)->svc->wait_contains_transactions && !QISEMPTY(ADVERT(p)->svc->waitq))
			if (earliest == 0 || timercmp(&FIRSTTRAN(earliest)->arrival, &FIRSTTRAN(ADVERT(p))->arrival, <))
				earliest = ADVERT(p);
}


static void add_to_waitq(const void *p, VISIT which, int depth)
{
	if (which == preorder || which == leaf)
	{
		debug(2, "%d waiting for %.*s", ADVERT(p)->client->fd, XATMI_SERVICE_NAME_LENGTH, ADVERT(p)->svc->svcname);
		ADVERT(p)->svc->wait_contains_transactions = 0;
		qappend(QLAST(ADVERT(p)->svc->waitq), &ADVERT(p)->svc_waitq);
		assert(!QISEMPTY(ADVERT(p)->svc->waitq));
	}
}


static void remove_from_waitq(const void *p, VISIT which, int depth)
{
	if (which == preorder || which == leaf)
		qdelete(&ADVERT(p)->svc_waitq);
}


void send_to_server(struct tpadvertisement *a, struct tptransaction *t)
{
	assert(a && a->client && a->svc && t && t->svc);
	debug(1, "send_to_server(%d, %.*s, %d)", a->client->fd, XATMI_SERVICE_NAME_LENGTH, a->svc->svcname, t->cd);
	struct tpclient *c = a->client;

	/* remove worker from all service waitq */
	twalk(c->adverts, remove_from_waitq);

	/* transaction meet worker, worker meet transaction */
	c->wait4 = t;
	t->executor = a;

	/* client must already be waiting for service request */
	svrwritel(c, a->opaque);
	svrwrite(c, t->svc->svcname, XATMI_SERVICE_NAME_LENGTH);
	svrwritel(c, tptypes(t->data, 0, 0));
	svrwritel(c, t->flags);
	svrwrites(c, t->cd);
	svrwrite(c, t->data, tptypes(t->data, 0, 0));
	svr_next_command(c);
}

void svrservice(struct tpclient *c, char *buf)
{
	assert(c);
	debug(1, "svrservice(%d)", c->fd);
	tpfree(buf);
	earliest = 0;
	twalk(c->adverts, find_earliest);
	if (earliest)
	{
		debug(1, "got immediate %.*s for %d", XATMI_SERVICE_NAME_LENGTH, FIRSTTRAN(earliest)->svc->svcname, c->fd);
		send_to_server(earliest, FIRSTTRAN(earliest));
	}
	else
	{
		debug(2, "%d waiting for work", c->fd);
		twalk(c->adverts, add_to_waitq);		/* nothing to do.  leave server waiting... */
	}
}
