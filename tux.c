/*
** tux.c -- connect stdin & stdout to a xatmi switchboard
**
** this is a diagnostic function.  it permits direct access to the switchboard's
** socket.
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

static char *sccsid = "@(#) tux.c 1.1 93/09/08 15:28:08";


#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

#include "tx.h"

extern _tx_server;

static void dump(char *m, char *p, int len)
{
	char hex[] = "0123456789ABCDEF";
	fprintf(stderr, "%s: >", m);
	while (len)
	{
		if (*p >= ' ' && *p < 127)
			fputc(*p, stderr);
		else
			fprintf(stderr, "\\x%c%c", hex[(*p >> 4) & 0x0f], hex[*p & 0x0f]);
		p++, len--;
	}
	fprintf(stderr, "<\n");
}

main(int argc, char *argv[])
{
	fd_set in, except;
	int debug, result, stdin_ok, sock_ok, len;
	char line[1024];
	
	if (argc < 2 || strcmp(argv[1], "-d"))
		debug = 0;
	else
	{
		debug = 1;
		argc--;
		argv++;
	}

	if (argc != 2 || argv[1][0] == '-')
	{
		fprintf(stderr, "usage: tux [-d] switchboard\n");
		exit(1);
	}

	if (tx_open(argv[1]) != TX_OK)
	{
		perror(argv[1]);
		exit(3);
	}
	if (debug) fprintf(stderr, "connected\n");

	stdin_ok = sock_ok = 1;
	while (stdin_ok && sock_ok)
	{
		FD_ZERO(&except); FD_SET(_tx_server, &except); FD_SET(0, &except);
		FD_ZERO(&in);
		if (stdin_ok) FD_SET(0, &in);
		if (sock_ok) FD_SET(_tx_server, &in);
		result = select(_tx_server + 1, &in, (fd_set *) 0 , &except, (struct timeval *) 0);
		if (result == -1) perror("select");
		if (FD_ISSET(0, &in))
		{
			len = read(0, line, sizeof(line));
			if (len < 1)
			{
				if (len == -1) perror("read stdin");
				stdin_ok = 0;
			}
			else if (tx_write(line, len) != TX_OK)
			{
				perror("tx_write");
				sock_ok = 0;
			}
			else if (debug)
				dump("sent", line, len);
		}
		if (FD_ISSET(_tx_server, &in))
		{
			len = read(_tx_server, line, sizeof(line));
			if (len < 1)
			{
				if (len == -1) perror("read switchboard");
				sock_ok = 0;
			}
			else if (write(1, line, len) != len)
			{
				perror("write");
				sock_ok = stdin_ok = 0;
			}
			else if (debug)
				dump("received", line, len);
		}
		if (FD_ISSET(_tx_server, &except))
		{
			fprintf(stderr, "exceptional condition on switchboard!\n");
			stdin_ok = sock_ok = 0;
		}
		if (FD_ISSET(0, &except))
		{
			fprintf(stderr, "exceptional condition on stdin!\n");
			stdin_ok = sock_ok = 0;
		}
	}
	tx_close();
	if (debug) fprintf(stderr, "tux exit\n");
	exit(0);
}
