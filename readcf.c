
/*
 * $Header: /tmp/cvs/psimail/readcf.c,v 1.2 1991/11/22 13:26:16 hans Exp $
 *
 */

/************ readcf()  ---> reads config file ********************

  Example of configfile :


#
# PSI-Mail Configuration
#

PSINUA=022846231008
POSTMASTER=POSTMASTER
SENDMAIL=/usr/lib/sendmail -odi -f


*******************************************************************/

/*
 *
 *	$Log: readcf.c,v $
 *	Revision 1.2  1991/11/22 13:26:16  hans
 *	SunLink/X25 changes.
 *	Misc. bug fixes
 *
 * Revision 1.1  1991/11/20  10:19:50  hans
 * Initial revision
 *
 *
 */

#ifndef lint
static char RCSid[] = "$Id: readcf.c,v 1.2 1991/11/22 13:26:16 hans Exp $ netCS";
#endif /* lint */

#include "psi_config.h"
#include <stdio.h>
#include <string.h>

char cf_mynua[128];
char cf_postmaster[128];
char cf_mta[128];

readcf()
{
	FILE *fpcf;
	char line[128];
	char *pos;
	char *dest;

	/* set defaults */
	strcpy(cf_mynua,"");
	strcpy(cf_postmaster,"postmaster");
	strcpy(cf_mta,"/usr/lib/sendmail -bm -odi -f");

	if(fpcf = fopen(CONFIGFILE,"r"))
	{
		while(fgets(line,127,fpcf))
		{
			if(*line == '\n' || *line == '#')
				continue;
			pos = strchr(line,'=');
			if(pos)
			{
				*pos++ = '\0';
				dest = (char *)0;
				if(!strcmp(line,"PSINUA"))
					dest = cf_mynua;
				if(!strcmp(line,"POSTMASTER"))
					dest = cf_postmaster;
				if(!strcmp(line,"SENDMAIL"))
					dest = cf_mta;
				if(dest)
				{
					strcpy(dest,pos);
					pos = strchr(dest,'\n');
					if(pos) *pos = '\0';
				}
			}
		}
		fclose(fpcf);
	}
}

