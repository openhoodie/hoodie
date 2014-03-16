/*
** the switchboard
**
** usage: tpserver [-d] switchboardname
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
**/

#include "server.h"


#include <signal.h>
#include <syslog.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>


static char *SWITCHBOARD;
static int master = -1;
int DEBUG = 0;


/************************************************************************************/


void debug(int level, char *fmt, ...)
{
	if (DEBUG >= level)
	{
		va_list ap;
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
		fprintf(stderr, "\n");
		fflush(stderr);
	}
}


void fatal(char *fmt, ...)
{
	va_list ap;
	if (DEBUG)
	{
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
		fprintf(stderr, "\n");
		fflush(stderr);
	}
	va_start(ap, fmt);
	vsyslog(LOG_ERR, fmt, ap);
	va_end(ap);
	exit(1);
}


void error(char *fmt, ...)
{
	va_list ap;
	if (DEBUG)
	{
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
		fprintf(stderr, "\n");
		fflush(stderr);
	}
	va_start(ap, fmt);
	vsyslog(LOG_ERR, fmt, ap);
	va_end(ap);
}


/************************************************************************************/

static void DumpFD(int fd)
{
	struct timeval timeout = { 1, 0 };
	fd_set readfds;
	char buf[500];
	int len;

	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);

	debug(0, "DumpFD: select on fd %d", fd);
	/* wait for something to do */
	if (select(fd + 1, &readfds, 0, 0, &timeout) == -1)
		error("select: %m");

	else if (FD_ISSET(fd, &readfds))
	{
		if ((len = read(fd, buf, sizeof buf)) == -1)
			error("Iowait: read: %m");
		else if (len == 0)
			debug(0, "eof on %d", fd);
		else
			dump(fd, "dump...", buf, len);
	}
}

/* command table.  start at 0.  no gaps.  CO-ORDINATE WITH MESSAGE TYPES in libxatmi.h. */
static struct {
	void (*func)(struct tpclient *, char *);
	char *func_name;
	int len;
} commands[] = {
	{svracall,	"svracall",       XATMI_SERVICE_NAME_LENGTH + IOLONG * 2},
	{svrgetrply,	"svrgetrply",     IOSHORT + IOLONG},
	{svradvertise,	"svradvertise",   XATMI_SERVICE_NAME_LENGTH + IOLONG},
	{svrunadvertise,"svrunadvertise", XATMI_SERVICE_NAME_LENGTH},
	{svrservice,	"svrservice",     0},
	{svrreturn,	"svrreturn",      1 + IOLONG * 2}
};

static void command(struct tpclient *c, char *buf)
{
	char cmd = *buf;
	if (cmd < sizeof commands / sizeof *commands)
		svriowait(c, tprealloc(buf, commands[cmd].len), commands[cmd].func, commands[cmd].func_name);
	else
	{
		error("unknown command %d", cmd);
		if (DEBUG > 2) DumpFD(c->fd);
		tpfree(buf);
		del_client(c);
	}
}

void svr_next_command(struct tpclient *c)
{
	svriowait(c, tpalloc(0, 0, 1), command, "command");
}


/************************************************************************************
 *****	Iowait waits for something to become available to read from any socket.
 *****	If something is available from the master socket a new client is connected.
 *****	If something is available from a client, it is read into a tuxedo buffer
 *****	and handed to a callback function when the buffer is full.  The handler is
 *****	responsible for tpfreeing the buffer.  Exceptions are handled for client
 *****	EOF and unexpected input.
 ************************************************************************************/

static void Iowait(void)
{
	int maxfd;
	fd_set readfds;
	QUEUE *q;

	/* accept input on any descriptor */
	FD_ZERO(&readfds);
	FD_SET(master, &readfds);
	maxfd = master;
	for (q = QFIRST(tpclients); q != &tpclients; q = QNEXT(q))
	{
		struct tpclient * c = OFFSET(q, struct tpclient, global);
		if (c->fd > maxfd) maxfd = c->fd;
		FD_SET(c->fd, &readfds);
	}

debug(3, "entering select to fd %d", maxfd);
	/* wait for something to do */
	if (select(maxfd + 1, &readfds, 0, 0, 0) == -1)
	{
		error("select: %m");
		return;
	}

	/* new client connection */
	if (FD_ISSET(master, &readfds))
	{
debug(1, "new connection");
		int i = accept(master, 0, 0);
		if (i == -1)
			error("accept: %m");
		else
			svr_next_command(new_client(i));
	}

	q = QFIRST(tpclients);
	while (q != &tpclients)
	{
		struct tpclient * c = OFFSET(q, struct tpclient, global);
		q = QNEXT(q);
		if (FD_ISSET(c->fd, &readfds))
		{
			long bufsize = c->input ? tptypes(c->input, 0, 0) : 0;
			int used = c->used;
			if (used >= bufsize)	// something unexpected.  could be EOF
			{
				char check_eof[100];
				int len = read(c->fd, &check_eof, sizeof check_eof);
				if (len > 0)
				{
					error("Iowait: read %d unexpected bytes from fd %d", len, c->fd);
					dump(c->fd, "unexpected", check_eof, len);
				}
				del_client(c);
			}
			else if ((used = read(c->fd, c->input + used, bufsize - used)) == -1)
			{
				error("Iowait: read: %m");
				del_client(c);
			}
			else if (used == 0)
			{
				debug(1, "eof on %d", c->fd);
				del_client(c);
			}
			else if ((c->used = c->used + used) == bufsize)
			{
				void (*handle)(struct tpclient *, char *) = c->handler;
				char *buf = c->input;
				char *handler_name = c->handler_name;
				c->handler_name = 0;
				c->handler = 0;
				c->input = 0;
				c->used = 0;
				debug(1, "%d bytes read from %d.  calling handler %s at 0x%x", bufsize, c->fd, handler_name, handle);
				if (DEBUG > 2) dump(c->fd, "read", buf, bufsize);
				handle(c, buf);
				debug(2, "back from handler %s at 0x%x", handler_name, handle);
			}
			else
				debug(2, "%d bytes read from %d.  waiting for %d more...", used, c->fd, bufsize - c->used);
		}
if (q != &tpclients) debug(2, "-------");
	}
}



/************************************************************************************/



static void usage(void)
{
	fprintf(stderr, "usage: tpserver [-d] switchboardname\n");
	exit(1);
}


static void Arguments(int argc, char *argv[])
{
	int ch;
	for (;;)
	{
		ch = getopt(argc, argv, "d");
		if (ch == -1) break;
		switch (ch)
		{
			case 'd':
				DEBUG++;
				break;
			default:
				usage();
		}
	}

	if (optind != argc - 1) usage();
	SWITCHBOARD = argv[optind++];
}


static void exit_handler(int sig)
{
	unlink(SWITCHBOARD);
	close(master);
	exit(0);
}


static void trap_signals(void)
{
	signal(SIGHUP,  exit_handler);
	signal(SIGINT,  exit_handler);
	signal(SIGQUIT, exit_handler);
	signal(SIGTERM, exit_handler);
}


/************************************************************************************/


main(int argc, char *argv[])
{
	openlog("tpserver", LOG_PID|LOG_PERROR, LOG_DAEMON);
	Arguments(argc, argv);

	SWITCHBOARD = strdup(SWITCHBOARD);	/* I want to be able to modify it */
	char *p = strrchr(SWITCHBOARD, ':');
	if (!p)
	{	/* AF_UNIX address */
		master = socket(PF_UNIX, SOCK_STREAM, 0);
		if (master == -1)
			fatal("socket: %m");

		struct sockaddr_un my_addr = { AF_UNIX };
		strncat(my_addr.sun_path, SWITCHBOARD, sizeof(my_addr.sun_path));
		unlink(SWITCHBOARD);
		if (bind(master, (struct sockaddr *)&my_addr, sizeof my_addr) == -1)
			fatal("bind to %s: %m", my_addr.sun_path);
	}
	else
	{	/* AF_INET address */
		*p++ = 0;
		struct servent *service = getservbyname(p, "tcp");
		struct sockaddr_in my_addr;
		bzero(&my_addr, sizeof(my_addr));

		struct hostent *host = gethostbyname(SWITCHBOARD);
		if (!host)
			fatal("%s: %s", SWITCHBOARD, hstrerror(h_errno));
		bcopy(host->h_addr, &my_addr.sin_addr, host->h_length);
		my_addr.sin_family = host->h_addrtype;
		endhostent();

		my_addr.sin_port = service? service->s_port: htons(atoi(p));
		if (!ntohs(my_addr.sin_port))
			fatal("%s: No such service", p);
		endservent();

		master = socket(AF_INET, SOCK_STREAM, 0);
		if (master == -1)
			fatal("socket: %m");

		if (bind(master, (struct sockaddr *)&my_addr, sizeof my_addr) == -1)
		{
			p[-1] = ':';
			fatal("bind to %s: %m", SWITCHBOARD);
		}
	}

	if (listen(master, 5) == -1)
		fatal("listen to %s: %m", SWITCHBOARD);

	if (!DEBUG)
		switch (fork())
		{
		case -1:
			error("fork");
			exit(2);
		case 0:
			close(0);
			close(1);
			close(2);
			setsid();
			break;
		default:
			exit(0);
		}
	trap_signals();
	for (;;)
	{
		Iowait();
		debug(1, "------------------------------------------");
	}
	debug(1, "tpserver exit.  why?");
	return 15;
}
/*
** this is NOT standard.  it's how you connect to the system.
** pick an address, any address you like, and use it for the tpserver and all clients in the application.
** if the address contains a ":" it's interpreted as host:port for AF_INET, otherwise as a AF_UNIX address.
** In tpserver instantiation, host only makes sense if it's 0.0.0.0, or one of the local hosts addresses.
*/
#include <tx.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
