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
#include <stdlib.h>

#define NUM_FREE_CD 10		/* number of cd's to generate when we need more */

static QUEUE tptransactions = { &tptransactions, &tptransactions };

struct tptransaction *new_transaction(struct tpclient *c, long flags, struct tpservice *svc, long len)
{
	assert(c);
	if (!svc) return 0;
	debug(1, "new_transaction(%d, %.*s, %d)", c->fd, XATMI_SERVICE_NAME_LENGTH, svc->svcname, len);

	struct tptransaction *t;
	t = malloc(sizeof(struct tptransaction));
	if (!t) debug(2, "new_transaction(%d, %.*s, %d) -> 0x%x", c->fd, XATMI_SERVICE_NAME_LENGTH, svc->svcname, len, 0);
	if (!t) return 0;

	gettimeofday(&t->arrival, 0);
	QINIT(t->clientq);
	QINIT(t->svc_waitq);
	t->executor = 0;
	t->client = c;
	t->flags = flags;
	t->svc = svc;
	t->data = tpalloc(0, 0, len);
	if (!t->data)
	{
		free(t);
		debug(2, "new_transaction(%d, %.*s, %d) -> 0x%x", c->fd, XATMI_SERVICE_NAME_LENGTH, svc->svcname, len, 0);
		return 0;
	}

	/*
	 * cd's are random numbers, unique over all current transactions.  when we need one
	 * we pick the next random number of a list.  when the list is empty we fill it with
	 * new random numbers.  hope we don't have huge numbers of outstanding transactions.
	 */
	/* generate a new cd */
	static short free_cd[NUM_FREE_CD] = { 0 };
	static short *cdp = free_cd;
	while (*cdp == 0)
	{
		if (cdp - free_cd < NUM_FREE_CD)
			cdp++;

		/* list exhausted.  generate new one */
		else
		{
			/* start with random numbers */
			srand(time(0));
			for (cdp = free_cd; cdp - free_cd < NUM_FREE_CD; cdp++)
				*cdp = rand();
			/* prune entries for current transactions */
			QUEUE *p;
			for (p = QFIRST(tptransactions); p != &tptransactions; p = QNEXT(p))
			{
				short cd = OFFSET(p, struct tptransaction, global)->cd;
				for (cdp = free_cd; cdp - free_cd < NUM_FREE_CD; cdp++)
					if (*cdp == cd)
						*cdp = 0;
			}
			/* and we're done */
			cdp = free_cd;
		}
	}
	qappend(QLAST(tptransactions), &t->global);
	t->cd = *cdp;
	*cdp++ = 0;
	debug(2, "new_transaction(%d, %.*s, %d) -> 0x%x", c->fd, XATMI_SERVICE_NAME_LENGTH, svc->svcname, len, t);
	return t;
}

void del_transaction(struct tptransaction *t)
{
	if (t)
	{
		assert(t->svc && t->client);
		debug(1, "del_transaction(%d, %.*s, %d) 0x%x", t->client->fd, XATMI_SERVICE_NAME_LENGTH, t->svc->svcname, t->cd, t);
		qdelete(&t->clientq);
		qdelete(&t->svc_waitq);
		qdelete(&t->global);
		if (t->client->wait4 == t) t->client->wait4 = 0;
		if (t->executor && t->executor->client->wait4 == t) t->executor->client->wait4 = 0;
		tpfree(t->data);
		free(t);
	}
}
