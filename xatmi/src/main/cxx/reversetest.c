/*
** Demonstration client.
** It calls the REVERSE call, which reverses the characters in data, for each command-line argument
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
#include <string.h>
#include <tx.h>
#include <xatmi.h>
#include <stdio.h>


main(int argc, char **argv)
{
	if (argc-- < 2 || **++argv == '-')
	{
		write(2, "usage: reversetest switchboard string ...\n", 39);
		exit(1);
	}

	if (tx_open(*argv) < 0)
	{
		perror(*argv);
		exit(2);
	}

	while (--argc)
	{
		long l = strlen(*++argv);
		char *p = tpalloc(0,0,l);
		strncpy(p, *argv, l);
		if (tpcall("REVERSE", p, l, &p, &l, 0) == -1)
			perror("tpcall");
		else
			printf("REVERSE(%s) %s(%d, %ld) -> %.*s\n", *argv, (tperrno == TPESVCFAIL) ? "failed" : "succeeded", tperrno, tpurcode, (int)l, p);
	}

	tx_close();
	return 0;
}
