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
#include <signal.h>

extern int DEBUG;

void svrwrite(struct tpclient *c, void *val, int len)
{
	assert(c);
	if (DEBUG > 2) dump(c->fd, "wrote", val, len);
	void (*sigpipe)(int) = signal(SIGPIPE, SIG_IGN);
	int ret = write(c->fd, val, len);
	if (ret != len)
		error("write(%d, 0x%d, %d) returned %d.  expect to go down with the ship.", c->fd, val, len, ret);
	signal(SIGPIPE, sigpipe);
}


void svrwriteb(struct tpclient *c, unsigned char val)
{
	svrwrite(c, &val, 1);
}


void svrwrites(struct tpclient *c, unsigned short val)
{
	svrwriteb(c, val & 0xFF);
	svrwriteb(c, (val >> 8) & 0xFF);
}


void svrwritel(struct tpclient *c, unsigned long val)
{
	svrwrites(c, val & 0xFFFF);
	svrwrites(c, (val >> 16) & 0xFFFF);
}


void svriowait(struct tpclient *c, char *data, void (*func)(struct tpclient *, char *), char *handler_name)
{
	assert(c && data && func);
	assert(!(c->handler || c->input || c->used));
	debug(2, "%d wants %d bytes for %s at 0x%x", c->fd, tptypes(data,0,0), handler_name, func);
	if (tptypes(data, 0, 0))
	{
		c->handler_name = handler_name;
		c->handler = func;
		c->input = data;
		c->used = 0;
	}
	else
	{
		debug(2, "%d: calling handler %s immediately", c->fd, handler_name);
		func(c, data);
	}
}
