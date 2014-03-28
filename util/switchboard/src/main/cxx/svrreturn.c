// XXX don't like global_any_transaction
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

struct tptransaction global_any_transaction;	/* also used in svrgetreply */

static void part2(struct tpclient *c, char *buf)
{
	svr_next_command(c);

	struct tptransaction *t = c->wait4;
	c->wait4 = 0;
	t->executor = 0;
	if (t->client->wait4 == &global_any_transaction || t->client->wait4 == t)
	{
		debug(2, "%d sending reply to waiting client %d", c->fd, t->client->fd);
		snd_reply(t);
	}
	else
	{
		debug(2, "%d queueing reply", c->fd);
		qdelete(&t->clientq);
		qappend(QLAST(t->client->replies), &t->clientq);
	}
}

void svrreturn(struct tpclient *c, char *buf)
{
	assert(c && buf);
	struct tptransaction *t = c->wait4;
	t->rval = *buf;
	t->rcode = tolong(buf+1);
	t->data = tprealloc(t->data, tolong(buf+1+IOLONG));
	debug(1, "svrreturn(%d)", c->fd);
	tpfree(buf);
	svriowait(c, t->data, part2, "svrreturn_part2");
}
