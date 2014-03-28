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
#include <signal.h>
#include "tx.h"

#undef DEBUG
#ifdef DEBUG
#include <stdio.h>
#endif

extern _tx_server;


static void writei(int fd, char *s, int l, int i)
{
	int neg = i < 0;
	char b[10], *p;
	if (neg) i = -i;
	p = b + sizeof b;
	do
		*--p = '0' + i % 10;
	while (i /= 10);
	write(2, s, l);
	if (neg) write(2, "-", 1);
	write(2, p, b + sizeof b - p);
}

static void writex(int fd, char *s, int l, char *p)
{
	unsigned long i = (long)p;
	char b[10];
	p = b + sizeof b;
	do
		*--p = "0123456789abcdef"[i % 16];
	while (i /= 16);
	*--p = 'x';
	*--p = '0';
	write(2, s, l);
	write(2, p, b + sizeof b - p);
}


int tx_write(void *val, int len)
{
#ifdef DEBUG
	dump(_tx_server, "wrote", val, len);
#endif
	void (*sigpipe)(int) = signal(SIGPIPE, SIG_IGN);
	int ret = write(_tx_server, val, len);
	if (ret != len)
	{
		writei(2, "write(", 6, _tx_server);
		writex(2, ", ", 4, val);
		writei(2, ", ", 2, len);
		writei(2, ") returned ", 11, ret);
		write(2, ".  expect to go down with the ship.\n", 36);
	}
	signal(SIGPIPE, sigpipe);
	return (ret == len) ? TX_OK : TX_FAIL;
}


int tx_writeb(unsigned char val)
{
	return tx_write(&val, 1);
}


int tx_writes(unsigned short val)
{
	return (tx_writeb(val & 0xFF) == TX_OK && tx_writeb(val >> 8) == TX_OK)
		? TX_OK : TX_FAIL;
}


int tx_writel(unsigned long val)
{
	return (tx_writes(val & 0xFFFF) == TX_OK && tx_writes(val >> 16) == TX_OK)
		? TX_OK : TX_FAIL;
}


int tx_read(char *buf, int len)
{
	while (len > 0)
	{
		int i = read(_tx_server, buf, len);
		if (i < 1)
		{
			if (i < 0)
				perror("read");
#ifdef DEBUG
			else
			{
				fprintf(stderr, "eof on %d\n", _tx_server);
				fflush(stderr);
			}
#endif
			return TX_FAIL;
		}
#ifdef DEBUG
dump(_tx_server, "read", buf, i);
#endif
		buf += i;
		len -= i;
	}
	return TX_OK;
}
