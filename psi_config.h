

/* 
#define CONFIGFILE "/usr/lib/x25/psi/config"
#define OUT_LOG "/usr/lib/x25/psi/psiout.log"
#define	ERR_LOG		"/usr/lib/x25/psi/psierr.log"
*/

#define CONFIGFILE	 "/usr/local/lib/psi-config"
#define	IN_LOG		"/tmp/psiin.log"
#define OUT_LOG 	"/tmp/psiout.log"
#define	ERR_LOG		"/tmp/psierr.log"

#ifdef SUNLINK6
#define	PACKSIZE 128
#define BIG_ENDIAN
#endif
