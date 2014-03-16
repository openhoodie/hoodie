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


/*
 * ask for a reply.
 * unlike tpacall, which only sends data if the server says okay,
 * tpgetrply always accepts rlen bytes from the server.
 */

int tpgetrply(int *cd, char **data, long *len, long flags)
{
	char buf1[1+IOSHORT], buf2[IOLONG+IOLONG];
	char ok;
	short rd;

	if (cd == 0 || data == 0 || len == 0 || *len == 0)
		tperrno = TPEINVAL;

	else if ((tpurcode = tx_writeb(TPTGRPLY)) != TX_OK
	||	(tpurcode = tx_writes(*cd)) != TX_OK
	||	(tpurcode = tx_writel(flags)) != TX_OK
	||	(tpurcode = tx_read(buf1, 1+IOSHORT)) != TX_OK
	)
			tperrno = TPESYSTEM;

	else if (*buf1 != TPSUCCESS && *buf1 != TPFAIL)	/* server sets tperrno */
		tperrno = toshort(buf1+1);
	
	else if ((tpurcode = tx_read(buf2, IOLONG+IOLONG)) != TX_OK)
		tperrno = TPESYSTEM;

	else
	{
		tperrno = (*buf1 == TPFAIL) ? TPESVCFAIL : 0; /* ensure tperrno not TPESVCFAIL on TPSUCCESS */
		tpurcode = tolong(buf2);
		*len = tolong(buf2+IOLONG);
		if (*len)
		{
			*data = tprealloc(*data, *len);
			tx_read(*data, *len);
		}
		return *cd = toshort(buf1+1);
	}

	return -1;
}
