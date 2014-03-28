/* test echo server */
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
#include <tx.h>
#include <xatmi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


main(int argc, char **argv)
{
	char *service;

	if (argc < 3 || argv[1][0] == '-')
	{
		fprintf(stderr, "usage: tuxtest switchboard service string ...\n");
		exit(1);
	}

	argc--;
	if (tx_open(*++argv) < 0)
	{
		perror(*argv);
		exit(2);
	}

	argc--;
	service = *++argv;

	while (--argc)
	{
		long l = strlen(*++argv);
		char *p = tpalloc(0,0,l);
		strncpy(p, *argv, l);
		if (tpcall(service, p, l, &p, &l, 0) == -1)
			perror(service);
		else
			printf("%s %s(%d), tpurcode %d, %.*s\n", service, (tperrno == TPESVCFAIL) ? "failed" : "succeeded", tperrno, (int)tpurcode, (int)l, p);
	}

	tx_close();
	return 0;
}
