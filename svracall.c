// XXX send_to_worker shouldn't be here
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


/* send the transaction we just read to any available server */
/* queue the transaction if no server available */
static void send_to_worker(struct tpclient *c, char *buf)
{
	assert(c && buf);
	debug(1, "send_to_worker(%d)", c->fd);
	svr_next_command(c);

	/* most recent transaction */
	struct tptransaction *t = OFFSET(QLAST(c->requests), struct tptransaction, clientq);
	debug(2, "%d: cd for %.*s is %d", c->fd, XATMI_SERVICE_NAME_LENGTH, t->svc->svcname, t->cd);

	/* is any server waiting for one of these? */
	if (!QISEMPTY(t->svc->waitq) && !t->svc->wait_contains_transactions)
	{
		struct tpadvertisement *a = OFFSET(QFIRST(t->svc->waitq), struct tpadvertisement, svc_waitq);
		assert(a && a->client);
		debug(2, "%d: sending transaction to %d", c->fd, a->client->fd);
		send_to_server(a, t);
	}

	/* put this on the service wait queue */
	else
	{
		debug(2, "%d: adding transaction to service waitq", c->fd);
		t->svc->wait_contains_transactions = 1;
		qappend(QLAST(t->svc->waitq), &t->svc_waitq);
	}
}


void svracall(struct tpclient *c, char *buf)
{
	assert(c && buf);
	debug(1, "svracall(%d)", c->fd);
	struct tptransaction *t =
		new_transaction(
			c,
			tolong(buf+XATMI_SERVICE_NAME_LENGTH+IOLONG),
			fnd_service(buf),
			tolong(buf+XATMI_SERVICE_NAME_LENGTH));
	tpfree(buf);
	if (!t)
	{
		debug(1, "svracall: new_transaction(%d) failed: %m", c->fd);
		svrwriteb(c, -1);
		svrwrites(c, TPENOENT);
		svr_next_command(c);
	}
	else
	{
		qappend(QLAST(c->requests), &t->clientq);
		svrwriteb(c, 0);
		svrwrites(c, t->cd);
		svriowait(c, t->data, send_to_worker, "send_to_worker");
	}
}
