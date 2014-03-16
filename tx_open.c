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

int _tx_server = -1;

int tx_open(char *address)
{
	close(_tx_server);
	address = strdup(address);	/* I want to be able to modify it */

	char *p = strrchr(address, ':');
	if (!p)
	{	/* AF_UNIX address */
		struct sockaddr_un sun;
		sun.sun_family = AF_UNIX;
		strncpy(sun.sun_path, address, sizeof sun.sun_path);
		_tx_server = socket(AF_UNIX, SOCK_STREAM, 0);
		if (_tx_server != -1 && connect(_tx_server, (struct sockaddr *)&sun, sizeof sun) == -1)
		{
			close(_tx_server);
			_tx_server = -1;
		}
	}
	else
	{	/* AF_INET address */
		struct sockaddr_in sin;
		_tx_server = -1; // cheap error handling
		bzero(&sin, sizeof(sin));
		*p++ = 0;

		struct hostent *host = gethostbyname(address);
		if (!host) goto fail;
		bcopy(host->h_addr, &sin.sin_addr, host->h_length);
		sin.sin_family = host->h_addrtype;

		struct servent *service = getservbyname(p, "tcp");
		sin.sin_port = service? service->s_port: htons(atoi(p));
		if (!ntohs(sin.sin_port)) goto fail;

		_tx_server = socket(AF_INET, SOCK_STREAM, 0);
		if (_tx_server != -1 && connect(_tx_server, (struct sockaddr *)&sin, sizeof sin) == -1)
		{
			close(_tx_server);
			_tx_server = -1;
		}

	fail:
		endhostent();
		endservent();
	}

	free(address);
	return (_tx_server == -1) ? TX_FAIL : TX_OK;
}
