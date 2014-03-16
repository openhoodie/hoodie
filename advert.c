/*
** advertisement support for server
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
#include <string.h>
#include <search.h>


static int cmp_advertisement(const void *s1, const void *s2)
{
	assert(s1 && *(int**)s1 && s2 && *(int**)s2);
	debug(2, "cmp_advertisement(%.*s, %.*s) -> %d",
		XATMI_SERVICE_NAME_LENGTH, *(char**)s1,
		XATMI_SERVICE_NAME_LENGTH, *(char**)s2,
		strncmp(*(char**)s1, *(char**)s2, XATMI_SERVICE_NAME_LENGTH));
	return strncmp(*(char**)s1, *(char**)s2, XATMI_SERVICE_NAME_LENGTH);
}


/* find existing advertisement for current client. */
struct tpadvertisement *fnd_advertisement(struct tpclient *c, char *svcname)
{
	assert(c);
	debug(1, "fnd_advertisement(%d, %.*s)", c->fd, XATMI_SERVICE_NAME_LENGTH, svcname);
	struct tpservice *svc = fnd_service(svcname);
	struct tpadvertisement **a = svc ? tfind(&svc, &c->adverts, cmp_advertisement) : 0;
	debug(2, "fnd_advertisement(%d, %.*s) -> 0x%x", c->fd, XATMI_SERVICE_NAME_LENGTH, svcname, a ? *a : 0);
	return a ? *a : 0;
}


/* create new advertisement for client */
struct tpadvertisement *new_advertisement(struct tpclient *c, char *svcname, long opaque)
{
	assert(c);
	debug(1, "new_advertisement(%d, %.*s)", c->fd, XATMI_SERVICE_NAME_LENGTH, svcname);
	struct tpservice *svc = new_service(svcname);
	struct tpadvertisement *a;
	if (!svc)
		a = 0;
	else if ((a = malloc(sizeof(struct tpadvertisement))) != 0)
	{
		a->svc = svc;
		a->client = c;
		a->opaque = opaque;
		a->ttrans = a->tmsec = 0;
		QINIT(a->svc_waitq);
		struct tpadvertisement **a2 = tsearch(a, &c->adverts, cmp_advertisement);
		if (a2 == 0 || *a2 != a)
		{
			debug(1, "%d: existing advertisement found.  freeing new duplicate.", c->fd);
			free(a);
			a = a2 ? *a2 : 0;
		}
	}
	debug(2, "new_advertisement(%d, %.*s) -> 0x%x", c->fd, XATMI_SERVICE_NAME_LENGTH, svcname, a);
	return a;
}


/* delete advertisement */
void del_advertisement(struct tpadvertisement *a)
{
	if (a)
	{
		assert(a->client && a->svc);
		debug(1, "del_advertisement(%d, %.*s) 0x%x", a->client->fd, XATMI_SERVICE_NAME_LENGTH, a->svc->svcname, a);
		qdelete(&a->svc_waitq);
		tdelete(a, &a->client->adverts, cmp_advertisement);

		/* scan tpclients, looking for any other advertisement for this service */
		QUEUE *q;
		for (q = QFIRST(tpclients); q != &tpclients; q = QNEXT(q))
			if (fnd_advertisement(OFFSET(q, struct tpclient, global), a->svc->svcname))
				break;
		if (q == &tpclients)
			del_service(a->svc); /* nobody else advertises it */

		free(a);
	}
}
