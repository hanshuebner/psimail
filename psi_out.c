/*
 * $Header: /tmp/cvs/psimail/psi_out.c,v 1.2 1991/11/22 13:26:16 hans Exp $
 *
 *	psi_out.c	outgoing psimail handler
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
 *      900113  Patrick G.   General Cleanup, Library Support
 *      900124  Patrick G.   Default return Adress is now read from
 *                           /usr/lib/x25/psi/defaults
 *
 *	$Log: psi_out.c,v $
 *	Revision 1.2  1991/11/22 13:26:16  hans
 *	SunLink/X25 changes.
 *	Misc. bug fixes
 *
 * Revision 1.1  1991/11/20  10:22:23  hans
 * Initial revision
 *
 *
 */

#ifndef lint
static char     RCSid[] = "$Id: psi_out.c,v 1.2 1991/11/22 13:26:16 hans Exp $ netCS";
#endif				/* lint */

#include "psi_config.h"

#include <stdio.h>
#include <sysexits.h>
#ifdef SUNLINK6
#include <fcntl.h>
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

extern char *sys_errlist[];
extern int errno;

#else /* SUNLINK6 */
#include <sys/types.h>
#ifndef USE_X25LIB
#include <x25.h>
#include <x25cmd.h>
#endif				/* USE_X25LIB */
#include <x25lib.h>
/* #include <x25alias.h> */
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#endif

char           *tempnam(), *getlogin(), *strchr();
int             ExitStat;
int             fd;
FILE           *debugf;

char            subject[128];
char            from[128];
char            *from_full;
char            *from_add;
char            to[128];
char            buf[BUFSIZ];
char            sendbuf[6 * PACKSIZE];
char            nua[128];

extern char     cf_mynua[];
extern char     cf_postmaster[];
extern char     cf_mta[];

char           *VMS_codes[] = {
	"NOSUCHUSER",
	"NOSUCHUSR",
	"NOSUCHOBJ",
	"NOSUCHNODE",
	"UNREACHABLE",
	"LINKEXIT",
	"SHUT",
	"PRV",
	"IVDEVNAM",
	"USERDSABL",
	(char *) NULL
};

int             UNIX_codes[] = {
	EX_NOUSER,
	EX_NOUSER,
	EX_UNAVAILABLE,
	EX_NOHOST,
	EX_TEMPFAIL,
	EX_TEMPFAIL,
	EX_TEMPFAIL,
	EX_NOPERM,
	EX_CANTCREAT,
	EX_CANTCREAT
};

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


#endif /* SUNLINK6 */

find_retcode(mess)

	char           *mess;
{
	static char     ecode[32];
	register        i;

	sscanf(mess, "%*c%*[^-]-%*c-%[^,],%*s", ecode);

	for (i = 0; VMS_codes[i]; ++i)
		if (!strcmp(ecode, VMS_codes[i]))
			return (UNIX_codes[i]);
	return (EX_UNAVAILABLE);
}


main(argc, argv)

	int             argc;
	char           *argv[];
{
	register int    i;
	FILE           *file, *file2, *fopen();
	char           *tempname, *c;
	int             blen;
	int             len;
	unsigned        flags;
	int             retcode = EX_OK;
	int             fail = -1;
	int             valid_users = 0;
#ifndef SUNLINK6
	struct x25alias *alias;
#else
	static char		userdata[] = "\377\0\0\0V3.0 MAIL-11";
	int				ud_len = 16;
	CONN_ADR        local;  /* local address */
	CONN_DB         remote; /* remote address and user data */
#endif
	long            tim;
	char           *ctime();

	if (argc < 3) {
		fprintf(stderr, "usage: %s nua user [user] ...\n", *argv);
		exit(EX_USAGE);
	}
	readcf();
	debugf = fopen(OUT_LOG, "a");
	if (debugf)
		setbuf(debugf, NULL);
	else {
		perror(OUT_LOG);
		exit(1);
	}
	fprintf(debugf, "================================================\n");
	time(&tim);
	fprintf(debugf, "%s", ctime(&tim));
	for (i = 1; i < argc; ++i)
		fprintf(debugf, "%s ", argv[i]);
	fprintf(debugf, "\n");

	for (i = strlen(argv[1]) - 1; i >= 0; --i)
		argv[1][i] = tolower(argv[1][i]);

#ifndef SUNLINK6
	if (alias = x25getalnam(argv[1]))
		sprintf(nua, "%s,\\377\\0\\0\\0V3.0 MAIL-11",
			alias->xa_nua);
	else {
		int             i;

		for (i = 0; argv[1][i]; i++)
			if (!isdigit(argv[1][i]))
				exit(EX_NOHOST);
		sprintf(nua, "%s,\\377\\0\\0\\0V3.0 MAIL-11", argv[1]);
	}
#else 
	strcpy(nua, argv[1]);
#endif

	tempname = tempnam("/tmp", "psio_");
	if ((file = fopen(tempname, "w+")) == NULL) {
		perror(tempname);
		exit(EX_CANTCREAT);
	} else
		unlink(tempname);

	strcpy(subject, "(none)");	/* preset values */
	sprintf(from, "PSI%%%s::%s", cf_mynua, cf_postmaster);
	sprintf(to, "PSI%%%s::%s", argv[1], argv[2]);

	while (gets(buf)) {
		if (!strncmp(buf, "Subject: ", 9)) {
			strcpy(subject, &buf[9]);
			continue;	/* discard header line */
		}
		if (!strncmp(buf, "From: ", 6)) {
			convert_address(&buf[6], &from_add, &from_full);
			if (strchr(from_add, '!') || strchr(from_add, '@')) {
				if (from_full[0])
					sprintf(from, "\"%s\" (%s)", from_add,
						from_full);
				else
					sprintf(from, "\"%s\"", from_add);
			} else {
				if (from_full[0])
					sprintf(from, "%s (%s)", from_add,
						from_full);
				else
					strcpy(from, from_add);
			}
			continue;	/* discard header line */
		}
		if (!strncmp(buf, "To: ", 4)) {
			strcpy(to, &buf[4]);
			continue;	/* discard header line */
		}
		fprintf(file, "%s\n", buf);
		if (!strlen(buf)) {
			break;
		}
	}
	while (gets(buf)) {
		fprintf(file, "%s\n", buf);
	}
	rewind(file);


#ifndef SUNLINK6
	if ((fd = x25connect(nua)) < 0) {
		fprintf(stderr, "No connection made, exitstat = %d (%s)\n",
			x25_errno, _x25errlist[x25_errno]);
		fprintf(debugf, "No connection made, exitstat = %d (%s)\n",
			x25_errno, _x25errlist[x25_errno]);
		fprintf(debugf, "------------------------------------------------\n");
		exit(x25_errno == 8 ? EX_UNAVAILABLE : EX_TEMPFAIL);
					/*	  ^ ???? */
	}
#else
	if ((fd = socket(AF_X25, SOCK_STREAM, 0)) < 0) {
		fprintf(debugf, "socket: %s\n", sys_errlist[errno]);
		fprintf(debugf,
				"------------------------------------------------\n");
		perror(socket);
		exit(EX_OSERR);
	}
	if (strlen(cf_mynua) > 0) {
		local.hostlen = pack(cf_mynua, local.host);
		if (local.hostlen < 0) {
			fprintf(stderr, "invalid local address %s in config\n",
					cf_mynua);
			fprintf(debugf, "invalid local address %s in config\n",
					cf_mynua);
			fprintf(debugf,
					"------------------------------------------------\n");
			exit(EX_USAGE);
		}
		if (ioctl(fd, X25_WR_LOCAL_ADR, &local)) {
			fprintf(debugf, "ioctl(X25_WR_LOCAL_ADR): %s\n",
					sys_errlist[errno]);
			fprintf(debugf,
					"------------------------------------------------\n");
			perror("ioctl(X25_WR_LOCAL_ADR)");
			exit(EX_OSERR);
		}
	}
	if ((remote.hostlen = pack(nua, remote.host)) < 0) {
		fprintf(stderr, "invalid remote address %s in config\n",
				cf_mynua);
		fprintf(debugf, "invalid remote address %s in config\n",
				cf_mynua);
		fprintf(debugf,
				"------------------------------------------------\n");
		exit(EX_UNAVAILABLE);
	}
	remote.datalen = ud_len;
	bcopy(userdata, remote.data, ud_len);

	if (connect(fd, &remote, sizeof(remote)) < 0) {
		int rcode;
		fprintf(debugf, "cannot connect socket: %s\n",
				sys_errlist[errno]);
		rcode = log_cause_and_diag(debugf, fd);
		log_cause_and_diag(stderr, fd);
		fprintf(debugf,
				"------------------------------------------------\n");
		perror("cannot connect");
		exit(rcode);
	}
		
#endif
	fprintf(debugf, "------------------------------------------------\n");

	put_x25("\3\0\0\7\0\0\0\0\1\0\0\0\2\2\0\0", 16, 0);
	len = get_x25(buf);

	/*
	 * if (memcmp("\3\0\0\7\0\0\0\0\2\0\0\0\2\2\0\0", buf, 16)) {
	 * fprintf(stderr, "invalid startup sequence...\n");
	 * exit(EX_PROTOCOL); }
	 */

	i = 0;
	for (c = from; *c; ++c) {
		if (*c == '(')
			++i;
		if (*c == ')')
			--i;
		*c = i ? *c : toupper(*c);
	}

	put_x25(from, strlen(from));

	fail = -1;
	for (i = 2; i < argc; ++i) {
		register char  *p;
		int             in_paren;

		in_paren = 0;
		for (c = argv[i]; *c; ++c) {
			if (*c == '(')
				++in_paren;
			if (*c == ')')
				--in_paren;
			*c = in_paren ? *c : toupper(*c);
		}

		put_x25(argv[i], strlen(argv[i]));
		len = get_x25(buf);
		if (memcmp(buf, "\1\0\0\0", 4)) {
/*
			fprintf(stderr, "NAK-code for User %s : ", argv[i]);
			for (i=0; i<len; i++)
				fprintf(stderr, "0x%02x ", 0xff & buf[i]);
			fprintf(stderr, "\n");
*/
			while ((len = get_x25(buf)) > 1) {
				buf[len] = '\0';
				fprintf(stderr, "%s\n", buf);
				retcode = find_retcode(buf);
			}
			fprintf(stderr, "\n");
		} else {
			fail = 0;
			++valid_users;
		}
	}
	if (fail)
		exit(retcode);

	put_x25("\0", 1);
/*
	i = 0;
	for(c=to; *c; ++c)
	{
		if (*c == '(') ++i;
		if (*c == ')') --i;
		*c = i ? *c : toupper(*c);
	}
*/
	put_x25(to, strlen(to));
	put_x25(subject, strlen(subject));

	c = sendbuf;
	blen = 0;
	while (fgets(buf, BUFSIZ, file)) {
		unsigned short  len = (unsigned short) strlen(buf);
		unsigned char   byte;

		buf[--len] = '\0';

#ifdef BIG_ENDIAN
		byte = (unsigned char) ((len >> 8) & 0xFF);
		*c++ = byte;
		byte = (unsigned char) (len & 0xFF);
		*c++ = byte;
#else
		byte = (unsigned char) (len & 0xFF);
		*c++ = byte;
		byte = (unsigned char) ((len >> 8) & 0xFF);
		*c++ = byte;
#endif
		blen += 2;

		strcpy(c, buf);
		blen += len;
		c += len;
		if (len % 2) {	/* stay word bound */
			*c++ = '\0';
			++blen;
		}
		if (blen >= 4 * PACKSIZE) {
			put_x25(sendbuf, 4 * PACKSIZE);
			blen -= 4 * PACKSIZE;
			if (blen)
				memcpy(sendbuf, sendbuf + (4 * PACKSIZE), blen);
			c = sendbuf + blen;
		}
	}
	if (blen)
		put_x25(sendbuf, blen);
	put_x25("\0", 1);

	for (i = 0; i < valid_users; ++i) {
		len = get_x25(buf);
		if (len < 0)
			perror("EOT xmission");
		if (len > 0 && memcmp(buf, "\1\0\0\0", 4)) {
/*
			fprintf(stderr,
			"NAK-code after text submission for %d. receipient: ",
				i+1);
			for (i=0; i<len; i++)
				fprintf(stderr, "0x%02x ", 0xff & buf[i]);
			fprintf(stderr, "\n");
*/

			while ((len = get_x25(buf)) > 1) {
				buf[len] = '\0';
				fprintf(stderr, "%s\n", buf);
				retcode = find_retcode(buf);
			}
			fprintf(stderr, "\n");
		}
	}
	exit(retcode);
}


get_x25(buf)
	char           *buf;
{
	int             i;
	int             retval;

	retval = read(fd, buf, BUFSIZ);
	if (debugf) {
		fprintf(debugf, "R-> >");
		for (i = 0; i < retval; i++)
			if (isprint(buf[i]) && buf[i] < '\177')
				fprintf(debugf, "%c", buf[i]);
			else
				fprintf(debugf, "[%02x]", 0xff & buf[i]);
		fprintf(debugf, "< (%d)\n", retval);
	}
	return (retval);
}

put_x25(buf, len)
	char           *buf;
	int             len;
{
	int             i;

	if (write(fd, buf, len) != len) {
/*	if (ioctl(fd, XIO_SEND_PACK, &xp) < 0)	{	*/
		if (debugf)
			fprintf(debugf, "XIO_SEND_PACK failed, errno=%d\n",
				errno);
	} else if (debugf) {
		fprintf(debugf, "<-T >");
		for (i = 0; i < len; i++)
			if (isprint(buf[i] & 0x7F) &&
			    (buf[i] & 0x7F) < '\177')
				fprintf(debugf, "%c", buf[i]);
			else
				fprintf(debugf, "[%02x]",
					0xff & buf[i]);
		fprintf(debugf, "< (%d)\n", len);
	}
}


#if 0

convert_address(b, a, f)
	char           *b, *a, *f;
{
	register char  *p, *q;
	static char     full[64], add[128];

	if (!(p = strchr(b, '<')) || !(q = strchr(p + 1, '>'))) {
		strcpy(a, b);
		*f = '\0';
		return;
	}
	strncpy(add, p + 1, (int) (q - p - 1));
	strncpy(full, b, (int) (p - b));
	p = full + strlen(full) - 1;
	while (*p && isspace(*p))
		*p-- = '\0';
	q = full;
	if (*p == '"' && *q == '"') {
		*p = '\0';
		*q++ = '\0';
	}
	strcpy(a, add);
	strcpy(f, q);
}

#else

extern char *malloc();

convert_address(full, address, name)
	char	*full;
	char	**address;
	char	**name;
{
	char	*br1, *br2;
	char	**in_bracket, **out_of_bracket;
	char	*buf;
	char	*p;

	while (isspace(*full)) {
		full++;
	}

	buf = malloc(strlen(full)+1);
	strcpy(buf, full);

	if ((br1 = strchr(buf, '('))
		&& (br2 = strchr(br1, ')'))) {		/* address (name) */

		in_bracket = name;
		out_of_bracket = address;
	} else if ((br1 = strchr(buf, '<'))
			   && (br2 = strchr(br1, '>'))) {	/* name <address> */
		in_bracket = address;
		out_of_bracket = name;
	} else {
		*address = buf;
		*name = "";
		return;
	}

	if (br1 != buf) {
		*out_of_bracket = buf;
		p = br1;
		while (isspace(*--p))
			;
		*++p = 0;
		
		*in_bracket = ++br1;
		*br2 = 0;
	} else {
		*in_bracket = ++br1;
		*br2++ = 0;
		while (isspace(*br2))
			br2++;
		*out_of_bracket = br2;
	}

	/* strip trailing white space */

	p = *name + strlen(*name) - 1;
	while (p != *name && isspace(*p)) {
		*p-- = 0;
	}

	/* strip double quotes */

	if (*p == '"' && **name == '"') {
		(*name)++;
		*p = 0;
	}
}


#endif
