/*
** service support for server
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


static void *services = 0;


static int cmp_service(const void *s1, const void *s2)
{
	assert(s1 && s2);
	debug(2, "cmp_service(%.*s, %.*s) -> %d",
			XATMI_SERVICE_NAME_LENGTH, s1,
			XATMI_SERVICE_NAME_LENGTH, s2,
			strncmp((char*)s1, (char*)s2, XATMI_SERVICE_NAME_LENGTH));
	return strncmp((char*)s1, (char*)s2, XATMI_SERVICE_NAME_LENGTH);
}

/* find existing service queue */
struct tpservice *fnd_service(char *svcname)
{
	assert(svcname);
	debug(1, "fnd_service(%.*s)", XATMI_SERVICE_NAME_LENGTH, svcname);
	struct tpservice **svc = tfind(svcname, &services, cmp_service);
	debug(2, "fnd_service(%.*s) -> 0x%x", XATMI_SERVICE_NAME_LENGTH, svcname, svc ? *svc : 0);
	return svc ? *svc : 0;
}

/* create new service queue.  return current queue if exists. */
struct tpservice *new_service(char *svcname)
{
	assert(svcname);
	debug(1, "new_service(%.*s)", XATMI_SERVICE_NAME_LENGTH, svcname);
	struct tpservice *svc = malloc(sizeof(struct tpservice));
	if (svc)
	{
		strncpy(svc->svcname, svcname, XATMI_SERVICE_NAME_LENGTH);
		QINIT(svc->waitq);
		svc->wait_contains_transactions = 0;
		svc->ttrans = svc->tmsec = 0;
		struct tpservice **svc2 = tsearch(svc, &services, cmp_service);
		if (svc2 == 0 || *svc2 != svc)
		{
			debug(1, "existing service found.  freeing new duplicate.");
			free(svc);
			svc = svc2 ? *svc2 : 0;
		}
	}
	debug(2, "new_service(%.*s) -> 0x%x", XATMI_SERVICE_NAME_LENGTH, svcname, svc);
	return svc;
}

void del_service(struct tpservice *svc)
{
	if (svc)
	{
		assert(services);
		debug(1, "del_service(%.*s) 0x%x", XATMI_SERVICE_NAME_LENGTH, svc->svcname, svc);
		if (svc->wait_contains_transactions)
			while (!QISEMPTY(svc->waitq))
				del_transaction(OFFSET(QFIRST(svc->waitq), struct tptransaction, svc_waitq));
		else
			while (!QISEMPTY(svc->waitq))
				del_advertisement(OFFSET(QFIRST(svc->waitq), struct tpadvertisement, svc_waitq));
		tdelete(svc, &services, cmp_service);
		free(svc);
	}
}
