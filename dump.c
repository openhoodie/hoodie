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
#include <stdio.h>

dump(int fd, char *msg, char *p, int len)
{
	char buf[100];
	while (len)
	{
		char *q = buf;
		while (len && q < buf + sizeof buf)
		{
			if (*p >= ' ' && *p <= '~')
				*q++ = *p;
			else if (q + 3 >= buf + sizeof buf)
				break;
			else
			{
				char hex[] = "0123456789ABCDEF";
				*q++ = '\\';
				*q++ = hex[(*p >> 4) & 0x0f];
				*q++ = hex[*p & 0x0f];
			}
			len--;
			p++;
		}
		fprintf(stderr, "%d: %s %.*s\n", fd, msg, q - buf, buf);
	}
	fflush(stderr);
}
