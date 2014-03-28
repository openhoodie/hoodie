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

void svradvertise(struct tpclient *c, char *buf)
{
	assert(c && buf);
	long opaque = tolong(buf + XATMI_SERVICE_NAME_LENGTH);
	debug(1, "svradvertise(%d, 0x%x)", c->fd, opaque);
	struct tpadvertisement *a = new_advertisement(c, buf, opaque);
	tpfree(buf);
	if (!a)
		svrwrites(c, TPESYSTEM);
	else if (a->opaque != opaque)
		svrwrites(c, TPEMATCH);
	else
		svrwrites(c, 0);
	svr_next_command(c);
}
