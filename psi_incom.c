/*
 * $Header: /tmp/cvs/psimail/psi_incom.c,v 1.2 1991/11/22 13:26:16 hans Exp $
 *
 *	psi_incom.c	incoming psimail handler
 *
 *					6-Jun-1988 Hans Huebner
 *
 *	Copyright 1988 by Hans Huebner, netmbx Berlin
 *	Copyright 1989/90 by netCS Informationstechnik GmbH, Berlin
 *	All rights reserved
 *
 *                 
 *      Modification History :
 *      Date    Author       Changes
 *      -----------------------------
 *      900113  Patrick G.   Added x25-lib support.
 *      900113  Patrick G.   Use uname() if /etc/systemid isn't here
 *      900120  Patrick G.   fixed Timzone problem (was always set to 'MES')
 *      900120  Patrick G.   reads config file to determine MTA
 *
 *	$Log: psi_incom.c,v $
 *	Revision 1.2  1991/11/22 13:26:16  hans
 *	SunLink/X25 changes.
 *	Misc. bug fixes
 *
 * Revision 1.1  1991/11/20  10:22:07  hans
 * Initial revision
 *
 *
 */

#ifndef lint
static char RCSid[] = "$Id: psi_incom.c,v 1.2 1991/11/22 13:26:16 hans Exp $ netCS";
#endif /* lint */

#include "psi_config.h"
#include <stdio.h>
#ifdef SUNLINK6
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <net/if.h>
#include <sundev/syncstat.h>
#include <netx25/x25_pk.h>
#include <netx25/x25_ctl.h>
#include <netx25/x25_ioctl.h>
#include <sys/wait.h>
#else
#include <sys/types.h>
#ifndef USE_X25LIB
#include <x25.h> 
#include <x25cmd.h>
#endif /* USE_X25_LIB */
#include <x25lib.h>
#include <x25alias.h>
#include <fcntl.h>
#include <sysexits.h>
#endif
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <sys/utsname.h>


FILE	*debugf = NULL;
char	*getenv();
int	receipients;
char	*remdte;


extern char cf_mynua[];
extern char cf_postmaster[];
extern char cf_mta[];

static char *
gettzone()
{
	struct tm *mytm;
	time_t now;

	
	time(&now);
	mytm = localtime(&now);
#ifdef sun
	return(mytm->tm_zone);
#else
	return(mytm->tm_name);
#endif
}

#ifdef SUNLINK6

/*
 * pack() takes as input a null-terminated ASCII string str
 * representing an X.121 address, and converts it to packed BCD form.
 * bcd points to the array containing the packed BCD output.  If the
 * address has invalid characters, -1 is returned.  Otherwise, the
 * length of the address (in BCD digits) is returned.
 */
int
pack(str, bcd)
        char    *str;
        u_char  *bcd;
{
        int     i, j;
        u_char  c;

        i = j = 0;
        while (str[i]) {
                if (i >= 15 || str[i] < '0' || str[i] > '9')
                        return(-1);
                c = str[i] - '0';
                if (i & 1)
                        bcd[j++] |= c;
                else
                        bcd[j] = c << 4;
                i++;
        }
        return(i);
}


/*
 * unpack() takes as input a packed BCD string (bcd) and
 * its length in BCD digits (len), and converts it to a null-terminated
 * ASCII string.  If the address is invalid, -1 is returned, otherwise
 * its length in BCD digits is returned.
 */
int
unpack(bcd, len, str)
        u_char  *bcd;
        int     len;
        char    *str;
{
        int     i, j;
        u_char  c;

        if (len > 15)
                return(-1);
        i = j = 0;
        while (i < len) {
                if (i & 1)
                        c = bcd[j++] & 0x0f;
                else
                        c = bcd[j] >> 4;
                if (c > 9)
                        return(-1);
                str[i++] = '0' + c;
        }
        str[i] = '\0';
        return(i);
}
#else /* SUNLINK6 */

#if defined(USE_X25LIB)

get_x25_err(fd)
int fd;
{
	x25status(fd);
	x25perror("x25 : ");
}

#else /* DONT_USE_X25_LIB */
int
get_x25_err(fd)

	int						fd;
{	struct	x25_channels	xc;
	int						retval;

	if (ioctl(fd, XIO_STATUS, &xc) < 0)
		return(EX_TEMPFAIL);

	switch (xc.xc_status) {
		case X25_CLR:	switch (xc.xc_diag1) {
							case CLR_INV:
							case CLR_ACC:
							case CLR_UNREACH:
							case CLR_NOREV:
							case CLR_UNKDEST:
							case CLR_NOSINGLE:
							case CLR_ERR:	retval = EX_UNAVAILABLE;
											errno = 0;
											break;
							case CLR_DTE:
							case CLR_BUSY:
							case CLR_NER:
							case CLR_DER:
							case CLR_RPE:
							default:	retval = EX_TEMPFAIL;
										break;
						}
						if (debugf)
							fprintf(debugf, "CLR: %d\n", (int)xc.xc_diag1);
						break;
		case X25_RES:
		case X25_RST:
		case X25_INT:	retval = EX_TEMPFAIL;
						break;
		default:		retval = EX_TEMPFAIL;
						if (debugf)
						fprintf(debugf, "x25smtp: SICC-Error (stat 0x%02x, diag1 0x%02x, diag2 0x%02x)\n",
							xc.xc_status, xc.xc_diag1,
							xc.xc_diag2);
						break;
	}
	return(retval);
}

#endif /* USE_X25LIB */
#endif /* SUNLINK6 */

int
get_x25(buf)

	char	*buf;
{	int	i;
	int	retval;

	retval = read(0, buf, BUFSIZ);
	if (retval == -1) {
#ifndef SUNLINK6
		get_x25_err(0);
#endif SUNLINK6
		return(0);
	}

	if (debugf) {
		fprintf(debugf, "R-> >");
		for (i=0; i<retval; i++)
			if (isprint(buf[i]) && buf[i]<'\177')
				fprintf(debugf, "%c", buf[i]);
			else
				fprintf(debugf, "[%02x]", buf[i] & 0xff);
		fprintf(debugf, "< (%d)\n", retval);
	}
	return(retval);
}

put_x25(buf, len)

	char	*buf;
	int	len;
{	int	i;

	len = (len > PACKSIZE) ? PACKSIZE : len;

	if (write(0, buf, len) < len)	{
		if (debugf)
			fprintf(debugf, "write failed, errno=%d\n",
				errno);
	} else
		if (debugf) {
			fprintf(debugf, "<-T >");
			for (i=0; i<len; i++)
				if (isprint(buf[i]&0x7F) &&
				    (buf[i]&0x7F)<'\177')
					fprintf(debugf, "%c", buf[i]);
				else
					fprintf(debugf, "[%02x]",
						0xff & buf[i]);
			fprintf(debugf, "< (%d)\n", len);
		}
}

char	command[128];
char	to[10][64];
char	full_from[128];
char	subject[128];
char	xvmsmailto[128];
char	from_user[128];

int
get_text(out)

	FILE	*out;
{static	char	buf[BUFSIZ];
	unsigned short	len;
	FILE	*tmp;
	char	*tmpname = tmpnam(NULL);

	if (!(tmp = fopen(tmpname, "w+"))) {
		return(1);
	} else
		unlink(tmpname);

	for (;;) {
		len = get_x25(buf);
		if ((len == 1 && !*buf) || !len)
			break;
		fwrite(buf, len, 1, tmp);
	}

	rewind(tmp);

	while (fread((char *)&len, sizeof(len), 1, tmp) == 1) {
#ifdef BIG_ENDIAN
		unsigned short tmpi = (len & 0xff00) >> 8;
		len <<= 8;
		len |= tmpi;
#endif
		fread(buf, len, 1, tmp);
		if (len) {
			fprintf(out, "%.*s\n", len, buf);
			if (len & 1)	/* word bound */
				fgetc(tmp); 
		} else
			fprintf(out, "\n", len, buf);
	}
	fclose(tmp);
	return(0);
}

get_header()

{	int		len;
static	char	buf[256];
	int		i, no;
	char	*strrchr(), *strchr();
	char	*fullname;
	char	*p, *q;

	len = get_x25(buf);	/* get sender				*/
	buf[len] = '\0';
	for (i=0; i<len; i++)			/* VMS uses only UPPERCASE */
		buf[i] = tolower(buf[i]);

	if ((fullname = strchr(buf, '\"')) && (fullname != buf)) {	
		/* go and find out fullname */
		*fullname++ = '\0';
		if (p = strchr(fullname, '\"'))
			*p = '\0';
		
		p = fullname;		/* Make reasonable names again */
		*p = toupper(*p);
		while (p = strchr(p, ' ')) {
			++p;
			*p = toupper(*p);
		}
	} 

	/* kill trailing spaces */

	if (p = strchr(buf, ' '))
		*p = '\0';

	/* clean up address */
	
	if (*buf == '"') {
		strcpy(buf, buf+1);
		if (p = strchr(buf, '"')) {
			*p = 0;
		}
	}

	while (p = strchr(buf, '@')) {
		*p++ = '%';
	}
		

	if (fullname)
		sprintf(full_from, "%s@%s.PSI (%s)", buf, remdte, fullname);
	else
		sprintf(full_from, "%s@%s.PSI", buf, remdte);
	strcpy(from_user, buf);

	len = get_x25(buf);	/* get receipients			*/
	no = 0;
	do {
	int in_quote;

		p = to[no];
		in_quote = 0;
		for (i=0; i<len; i++)
		{
			if (buf[i] == '"')
			{
				in_quote = !in_quote;
				continue;
			}
/*			*p++ = in_quote ? buf[i] : tolower(buf[i]);	*/
			*p++ = tolower(buf[i]);
		}
		*p = '\0';
		put_x25("\1\0\0\0", 4);
		len = get_x25(buf);
		no++;
		++receipients;
	} while (*buf && len > 1);
	to[no][0] = '\0';

	len = get_x25(xvmsmailto);
	xvmsmailto[len] = '\0';
	len = get_x25(subject);
	subject[len] = '\0';
}

char	temp[8][16];

#ifdef SUNLINK6
void
munch_zombies()
{
	int status;

	errno = 0;

	while (wait3(&status, WNOHANG, 0) > 0)
		;
}
#endif

main(argc, argv)

	int			argc;
	char		*argv[];
{	static	char		buf[BUFSIZ];
	static	char		myhostname[16];
	int		len;
	FILE		*out, *popen();
	unsigned	xf;
	char		*ctime();
	long		cur;
	int		i;
	char		*tbuf;
	char		msgid[100];
#ifndef SUNLINK6
	struct	x25alias	*alias;
#else
	int			ls;
	CONN_DB         bind_addr;	/* local address */
	CONN_DB         remote; 	/* remote address and user data */
	int             remote_len = sizeof(CONN_DB);
								/* remote address length */
	char			remdtebuf[64];
#endif /* SUNLINK6 */
	int debug = 0;

	if (argc > 1 && !strcmp(argv[1], "-d")) {
		debug = 1;
		argc--;
		argv++;
	}

	if (!debug) {
		close(1);
		open("/dev/null", 2);
	}

	readcf();

	if (out = fopen("/etc/systemid", "r")) {
		fgets(myhostname, sizeof(myhostname), out);
		myhostname[strlen(myhostname)-1] = '\0';
		fclose(out);
	} else
		myhostname[0] = '\0';


	if(strlen(myhostname)<3) {
		struct utsname myname;

		uname(&myname);
		strncpy(myhostname,myname.nodename,sizeof(myname.nodename));
		myhostname[sizeof(myname.nodename)] = '\0';
	}

	if (!debug) {
		close(2);
		fopen(ERR_LOG, "a");
	}	

	if (debug) {
		debugf = stderr;
	} else {
		debugf = fopen(argc != 1 ? argv[1] : IN_LOG, "a");
	}
	setbuf(debugf, NULL);

#ifdef SUNLINK6

	signal(SIGCHLD, munch_zombies);

	if ((ls = socket(AF_X25, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}
	if (strlen(cf_mynua)) {
		if ((bind_addr.hostlen = pack(cf_mynua, bind_addr.host)) < 0)
		{
			fprintf(stderr, "invalid (sub)address: %s\n", cf_mynua);
			exit(1);
		}
	} else {
		bind_addr.hostlen = ANY_SUBADDRESS;
	}
	bind_addr.hostlen |= ANY_LINK;
	memcpy(bind_addr.data, "\377\0\0\0", 4);
	bind_addr.datalen = 4;

	if (bind(ls, &bind_addr, sizeof(bind_addr)) < 0) {
		perror("bind");
		exit(1);
	}

	if (listen(ls, 5) < 0) {
		perror("listen");
		exit(1);
	}

#endif /* SUNLINK6 */

#ifndef SUNLINK6
#ifdef USE_X25LIB
	x25accept(0);
#else
	ioctl(0, XIO_ACCEPT, 0);
#endif /* USE_X25_LIB */
#else /* SUNLINK6 */

	do {
		int s;

		if ((s = accept(ls, &remote, &remote_len)) < 0) {
			perror("accept");
			exit(1);
		}

		close(0);
		dup(s);
	 

	} while (!debug && (fork() != 0));

#endif /* SUNLINK6 */

	len = get_x25(buf);	/* thats the identification line	*/
	buf[8] = '\002';	/* fiddle the ident			*/
	put_x25(buf, len);	/* and send it back			*/

#ifdef SUNLINK6
	unpack(remote.host, remote.hostlen, remdtebuf);
	remdte = remdtebuf;
#else
	remdte = getenv("FROM_DTE");
	if (alias = x25getalnua(remdte))
		remdte = alias->xa_name;
#endif

	get_header();

	time(&cur);
	tbuf = ctime(&cur);
	fprintf(debugf, "=================================================\n");
	fprintf(debugf, "%s%s@%s.PSI\n", tbuf, from_user, remdte);
	fprintf(debugf, "-------------------------------------------------\n");
	tbuf[24] = '\0';

/* Cryptical sendmail-date fromat: Wed, 10 Aug 88 01:23:54	*/
	sscanf(tbuf, "%s %s %s %s %s",	temp[0], temp[1], temp [2],
					temp[3], temp[4]);
	sprintf(tbuf, "%s, %s %s %s %s", temp[0], temp[2], temp[1],
					&temp[4][2], temp[3]);

	sprintf(command, "%s '%s@%s.PSI' ",cf_mta,
			from_user, remdte); 

	for (i=0; to[i][0]; i++) {
		strcat(command, "'");
		strcat(command, to[i]);
		strcat(command, "' ");
	}
	if ((out = popen(command, "w")) == NULL)
		exit(1);

	fprintf(out, "From %s %s remote from %s.PSI\n",
		      from_user, tbuf, remdte);
	sprintf(msgid, "P%05d.%09d", getpid(), time(0l));
	if (myhostname[0])
		fprintf(out,
#ifdef SUNLINK6
		"Received: by %s.PSI (SunLink/X.25 - psi_incom); %s %s; %s\n",
#else
		"Received: by %s.PSI (netX.25/psi_incom); %s %s; %s\n",
#endif
			myhostname, tbuf, gettzone(), msgid);	
	fprintf(out, "Date: %s %s\n", tbuf,gettzone());
	fprintf(out, "Message-Id: <%s@%s.PSI>\n", msgid, remdte);
	fprintf(out, "From: %s\n", full_from);
	fprintf(out, "To: ");
	for (i=0; to[i][0]; i++)
		fprintf(out, (i ? ", %s" : "%s"), to[i]);
	fprintf(out, "\n");
	fprintf(out, "Subject: %s\n", subject);
	fprintf(out, "X-PSI-Mail-To: %s\n", xvmsmailto);
	fprintf(out, "\n");

	get_text(out);
	pclose(out);

	for (i=0; i<receipients; ++i)
		put_x25("\1\0\0\0", 4);
}

