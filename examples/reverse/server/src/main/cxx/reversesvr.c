/*
** Demonstration server.
** It implements the REVERSE call, which reverses the characters in data
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

#include <stdlib.h>
#include <stdio.h>
#include <tx.h>
#include <xatmi.h>

static int rval = TPFAIL;

static void reversesvr(TPSVCINFO *info)
{
	char *p = info->data;
	char *q = p + tptypes(p, 0, 0) - 1;
	while (p < q)
	{
		char r = *p;
		*p++ = *q;
		*q-- = r;
	}
	rval = 3 - rval;	// flip flop
	tpreturn(rval, rval, info->data, info->len, 0);
}


main(int argc, char **argv)
{
	if (argc != 2 || argv[1][0] == '-')
	{
		write(2, "usage: reversesvr switchboard\n", 27);
		exit(1);
	}

	if (tx_open(argv[1]) < 0)
	{
		perror(argv[1]);
		exit(2);
	}

	if (tpadvertise("REVERSE", reversesvr) == -1)
	{
		write(2, "couldn't advertise REVERSE\n", 30);
		exit(3);
	}

	TPSVCINFO info;
	for (;;)
	{
		tpservice(&info);
		if (!info.cd)
			break;
		if (info.data)
			;
		printf("%d: %.*s\n", info.cd, XATMI_SERVICE_NAME_LENGTH, info.name);
	}

	tpunadvertise("REVERSE");
	tx_close();
	return 0;
}
