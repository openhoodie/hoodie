#include "byteorder.h"
/*
 * This file as per:
 *   The Open Group, Technical Standard
 *   Distributed Transaction Processing: The XATMI Specification
 */
#ifndef __XATMI_H__
#define __XATMI_H__

/*
 * flag bits for C language xatmi routines
 */
#define TPNOBLOCK	0x00000001
#define TPSIGRSTRT	0x00000002
#define TPNOREPLY	0x00000004
#define TPNOTRAN	0x00000008
#define TPTRAN		0x00000010
#define TPNOTIME	0x00000020
#define TPGETANY	0x00000080
#define TPNOCHANGE	0x00000100
#define TPCONV		0x00000400
#define TPSENDONLY	0x00000800
#define TPRECVONLY	0x00001000


/*
 * values for rval in tpreturn
 */
#define TPFAIL		0x0001
#define TPSUCCESS	0x0002


/*
 * service information structure
 */
#define XATMI_SERVICE_NAME_LENGTH 15	/* must be >= 15 */
typedef struct {
	char	name[XATMI_SERVICE_NAME_LENGTH];
	char	*data;
	long	len;
	long	flags;
	int 	cd;
} TPSVCINFO;


/*
 * globals
 */
extern int	tperrno;
extern long	tpurcode;


/*
 * error values for tperrno
 */
#define TPEBADDESC	2
#define TPEBLOCK	3
#define TPEINVAL	4
#define TPELIMIT	5
#define TPENOENT	6
#define TPEOS		7
#define TPEPROTO	9
#define TPESVCERR	10
#define TPESVCFAIL	11
#define TPESYSTEM	12
#define TPETIME		13
#define TPETRAN		14
#define TPGOTSIG	15
#define TPEITYPE	17
#define TPEOTYPE	18
#define TPEEVENT	22
#define TPEMATCH	23


/*
 * events returned during conversational communication
 */
#define TPEV_DISCONIMM	0x0001
#define TPEV_SVCERR	0x0002
#define TPEV_SVCFAIL	0x0004
#define TPEV_SVCSUCC	0x0008
#define TPEV_SENDONLY	0x0020


/*
 * X/Open defined typed buffers
 */
#define X_OCTET		"X_OCTET"
#define X_C_TYPE	"X_C_TYPE"
#define X_COMMON	"X_COMMON"


/*
 * xatmi communication api
 */

int tpacall(char *svc, char *data, long len, long flags);
int tpadvertise(char *svcname, void (*func)(TPSVCINFO *));
char *tpalloc(char *type, char *subtype, long size);
int tpcall(char *svc, char *idata, long ilen, char **odata, long *olen, long flags);
int tpcancel(int cd);
int tpconnect(char *svc, char *data, long len, long flags);
int tpdiscon(int cd);
void tpfree(char *ptr);
int tpgetrply(int *cd, char **data, long *len, long flags);
char *tprealloc(char *ptr, long size);
int tprecv(int cd, char **data, long *len, long flags, long *revent);
void tpreturn(int rval, long rcode, char *data, long len, long flags);
int tpsend(int cd, char *data, long len, long flags, long *revent);
void tpservice(TPSVCINFO *svcinfo);
long tptypes(char *ptr, char *type, char *subtype);
int tpunadvertise(char *svcname);


#endif /* __XATMI_H__ */
