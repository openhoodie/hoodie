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
#ifndef __QUEUE_H__
#define __QUEUE_H__

/*
 * a doubly-linked list for transaction queues
 *
 * a QUEUE is an ordered list of objects.  it is implemented as a doubly-linked circular list,
 * and requires space for two pointers in every object on the list, as well as two pointers
 * for the list itself.  The QUEUE struct provides pointers for both these cases.
 */


/* initialise queue headers to point to themselves */
typedef struct queue { struct queue *p1, *p2; } QUEUE;

/* (this is clever. can't remember where i saw it first.) */
#define OFFSETOF(type, field) ((char*)&((type*)0)->field - (char*)0)
#define OFFSET(ptr, type, field) ((type*)((char*)(ptr) - OFFSETOF(type, field)))

#define QINIT(q) ((q).p1 = (q).p2 = &(q))
#define QISEMPTY(q) ((q).p1 == &(q))
#define QFIRST(q) (q).p2
#define QPREV(q) (q)->p1
#define QNEXT(q) (q)->p2
#define QLAST(q) (q).p1

/* example list traversal: for (p = qfirst(q); p != &queue; p = qnext(q)) offset(p, struct mystruct, queue)->myfield++ */

QUEUE *qappend(QUEUE *, QUEUE *newnode);
QUEUE *qprepend(QUEUE *, QUEUE *newnode);
QUEUE *qdelete(QUEUE *node);

#endif /* __QUEUE_H__ */
