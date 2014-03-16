// XXX don't like global tpclients
/*
** client support for server
** 
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
#include <stdlib.h>

QUEUE tpclients = { &tpclients, &tpclients };

struct tpclient *new_client(int fd)
{
	debug(1, "new_client(%d)", fd);
	struct tpclient *c = malloc(sizeof(struct tpclient));
	if (c)
	{
		c->fd = fd;
		c->used = 0;
		c->input = 0;
		c->handler = 0;
		c->handler_name = 0;
		c->wait4 = 0;
		c->adverts = 0;
		QINIT(c->requests);
		QINIT(c->replies);
		qappend(QLAST(tpclients), &c->global);
	}
	debug(2, "new_client(%d) -> 0x%x", fd, c);
	return c;
}


void del_client(struct tpclient *c)
{
	if (c)
	{
		debug(1, "del_client(%d) 0x%x", c->fd, c);
		struct tptransaction global_any_transaction;        /* also used in svrgetreply */
		struct tptransaction *t = c->wait4;
		if (t && t->executor && t->executor->client == c)
		{
			assert(t->client && t->svc);
			/* what can we do with the transaction we're executing? */
			c->wait4 = 0;
			t->executor = 0;
			t->rval = TPERRNO;
			t->rcode = TPESVCERR;
			if (t->client->wait4 == &global_any_transaction || t->client->wait4 == t)
			{
				debug(1, "%d returning failed transaction to waiting client %d", c->fd, t->client->fd);
				snd_reply(t);
			}
			else
			{
				debug(1, "%d queueing failed transaction", c->fd);
				qdelete(&t->clientq);
				qappend(QLAST(t->client->replies), &t->clientq);
			}
		}
		while (c->adverts) del_advertisement(*(struct tpadvertisement**)c->adverts);
		while (!QISEMPTY(c->requests)) { QUEUE *q = QFIRST(c->requests); del_transaction(OFFSET(q, struct tptransaction, clientq)); }
		while (!QISEMPTY(c->replies )) { QUEUE *q = QFIRST(c->replies ); del_transaction(OFFSET(q, struct tptransaction, clientq)); }
		qdelete(&c->global);
		close(c->fd);
		free(c);
	}
}
