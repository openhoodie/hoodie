# 
# Copyright (c) 2007, DNA Pty Ltd and contributors
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

CFLAGS=-I. -g
LDFLAGS=-g
SOURCES=README COPYRIGHT CHANGELOG TODO *.c *.h Makefile
SERVER=	advert.o client.o queue.o service.o tpserver.o transaction.o svrio.o\
	svracall.o svradvertise.o svrgetrply.o svrreturn.o svrservice.o svrunadvertise.o
LIBXATMI=tpacall.o tpadvertise.o tpalloc.o tpcall.o tpcancel.o tpconnect.o tpdiscon.o tpfree.o tpgetrply.o\
	tpio.o tprealloc.o tprecv.o tpreturn.o tpsend.o tptypes.o tpunadvertise.o byteorder.o
LIBTX=	tx_begin.o tx_close.o tx_commit.o tx_info.o tx_open.o tx_io.o tx_rollback.o tx_set_commit_return.o\
	tx_set_transaction_control.o tx_set_transaction_timeout.o
TARGETS=server.a libxatmi.a libtx.a tpserver reversesvr reversetest tuxtest tux

all: $(TARGETS)

libxatmi.a: libxatmi.a($(LIBXATMI))
libxatmi.a($(LIBXATMI)): byteorder.h xatmi.h libxatmi.h tx.h

libtx.a: libtx.a($(LIBTX))
libtx.a($(LIBTX)): tx.h

server.a: server.a($(SERVER))
server.a($(SERVER)): byteorder.h queue.h xatmi.h server.h

tpserver: server.a libxatmi.a tx_open.o dump.o
	cc $(CFLAGS) -o $@ tpserver.c server.a libxatmi.a tx_open.o dump.o

tux: tux.c libtx.a tx.h
	cc $(CFLAGS) -o $@ tux.c libtx.a

reversesvr: reversesvr.c libxatmi.a libtx.a dump.o tx.h
	cc $(CFLAGS) -o $@ reversesvr.c libxatmi.a libtx.a dump.o

reversetest: reversetest.c libxatmi.a libtx.a dump.o tx.h
	cc $(CFLAGS) -o $@ reversetest.c libxatmi.a libtx.a dump.o

tuxtest: tuxtest.c libxatmi.a libtx.a tx.h
	cc $(CFLAGS) -o $@ tuxtest.c libxatmi.a libtx.a

clean:
	rm -f $(TARGETS) dump.o

ci:
	ci -l -m. $${TAG:+-n$$TAG} $(SOURCES)


diff:
	rcsdiff -kkvl $${TAG:+-r$$TAG} -u --ignore-all-space $(SOURCES)
