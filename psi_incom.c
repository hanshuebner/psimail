/*
 * $Header: /tmp/cvs/psimail/psi_incom.c,v 1.1 1991/11/20 10:22:07 hans Exp $
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
 *	Revision 1.1  1991/11/20 10:22:07  hans
 *	Initial revision
 *
 *
 */

#ifndef lint
static char RCSid[] = "$Id: psi_incom.c,v 1.1 1991/11/20 10:22:07 hans Exp $ netCS";
#endif /* lint */

#include <stdio.h>
#include <sys/types.h>
#ifndef USE_X25LIB
#include <x25.h> 
#include <x25cmd.h>
#endif /* USE_X25_LIB */
#include <x25lib.h>
#include <x25alias.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sysexits.h>
#include <sys/utsname.h>
#include <time.h>


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
	return(mytm->tm_name);
}

#ifdef USE_X25LIB

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

int
get_x25(buf)

	char	*buf;
{	int	i;
	int	retval;

	retval = read(0, buf, BUFSIZ);
	if (retval == -1) {
		get_x25_err(0);
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

	if (fullname = strchr(buf, '\"')) {	/* go and find out fullname */
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

main(argc, argv)

	int			argc;
	char		*argv[];
{static	char		buf[BUFSIZ];
static	char		myhostname[16];
	int		len;
	FILE		*out, *popen();
	unsigned	xf;
	char		*ctime();
	long		cur;
	int		i;
	char		*tbuf;
struct	x25alias	*alias;

	close(1);
	open("/dev/null", 2);
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

	close(2);
	fopen("/usr/lib/x25/psi/psierr.log", "a");
	if (argc != 1) {
		debugf = fopen(argv[1], "a");
		setbuf(debugf, NULL);
	}

#ifdef USE_X25LIB
	x25accept(0);
#else
	ioctl(0, XIO_ACCEPT, 0);
#endif /* USE_X25_LIB */

	len = get_x25(buf);	/* thats the identification line	*/
	buf[8] = '\002';	/* fiddle the ident			*/
	put_x25(buf, len);	/* and send it back			*/

	remdte = getenv("FROM_DTE");
	if (alias = x25getalnua(remdte))
		remdte = alias->xa_name;

	time(&cur);
	tbuf = ctime(&cur);
	fprintf(debugf, "=================================================\n");
	fprintf(debugf, "%s%s@%s.PSI\n", tbuf, from_user, remdte);
	fprintf(debugf, "-------------------------------------------------\n");
	tbuf[24] = '\0';

	get_header();

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
	if (myhostname[0])
		fprintf(out,
		"Received: by %s.PSI (net-x25/psi_in); %s %s; AA%05d\n",
			myhostname, tbuf, gettzone(), getpid());	
	fprintf(out, "Date: %s %s\n", tbuf,gettzone());
	fprintf(out, "Message-Id: <AA%05d@%s.PSI>\n", getpid(), remdte);
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

