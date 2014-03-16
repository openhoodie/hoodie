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
#include <assert.h>
#include <string.h>

long tptypes(char *ptr, char *type, char *subtype)
{
	if (IS_MAGIC(ptr))
	{
		struct tpmemory *p = (struct tpmemory *)ptr - 1;
		struct tpmemory *p2 = p + ALLOC_UNITS(p->len) - 1;
		assert(p->len == p2->len);
		assert(p->magic == p2->magic);
		assert(p->type  == p2->type);
		assert(p->subtype == p2->subtype);
		if (type) strncpy(type, p->type, TYPELEN);
		if (subtype) strncpy(subtype, p->subtype, SUBTYPELEN);
		return p->len;
	}

	tperrno = TPEINVAL;
	return -1;
}
