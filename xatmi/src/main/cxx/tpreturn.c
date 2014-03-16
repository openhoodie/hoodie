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
#include "libxatmi.h"
#include <setjmp.h>
#include <stdio.h>

static int _tp_longjmp_okay;
static jmp_buf _tp_jmpbuf;

/* not much point checking tx_write return code in this routine.  what could we do? */

void tpreturn(int rval, long rcode, char *data, long len, long flags)
{
	if (_tp_longjmp_okay)
	{
		tx_writeb(TPTRETN);
		tx_writeb((rval != TPFAIL && rval != TPSUCCESS) ? TPFAIL : rval);
		tx_writel(rcode);
		if (!data)
			tx_writel(0);
		else
		{
			tx_writel(len);
			tx_write(data, len);
		}
		longjmp(_tp_jmpbuf, 1);
	}
}


void tpservice(TPSVCINFO *svcinfo)
{
	if (svcinfo)
	{
		tx_writeb(TPTWORK);
		char opaque[IOLONG];
		if ((tpurcode = tx_read(&opaque, IOLONG)) != TX_OK
		|| (tpurcode = tx_read(svcinfo->name, XATMI_SERVICE_NAME_LENGTH + IOLONG * 2 + IOSHORT)) != TX_OK)
		{
			tperrno = TPESYSTEM;
			svcinfo->cd = 0;
			return;
		}

		/*
		 * converting the following fields in-situ.  convert from end-of-structure towards
		 * start-of-structure, because positions *will* change.
		 */
		svcinfo->cd = toshort(svcinfo->name + XATMI_SERVICE_NAME_LENGTH + IOLONG * 2);
		svcinfo->flags = tolong(svcinfo->name + XATMI_SERVICE_NAME_LENGTH + IOLONG);
		svcinfo->len = tolong(svcinfo->name + XATMI_SERVICE_NAME_LENGTH);
		void (*func)(TPSVCINFO*) = (void(*)(TPSVCINFO*))tolong(&opaque[0]);
		svcinfo->data = tpalloc(0, 0, svcinfo->len);
		if ((tpurcode = tx_read(svcinfo->data, svcinfo->len)) != TX_OK)
		{
			tperrno = TPESYSTEM;
			svcinfo->cd = 0;
			return;
		}

		_tp_longjmp_okay = 1;
		if (!setjmp(_tp_jmpbuf))
		{
			func(svcinfo);
			tx_writeb(TPTRETN);	/* they fell out of the service routine */
			tx_writeb(TPERRNO);
			tx_writel(TPESVCERR);
			tx_writel(0);
		}
		_tp_longjmp_okay = 0;
		tpfree(svcinfo->data);
		svcinfo->data = 0;
	}
}
