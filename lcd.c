/* lcd.c - log cause and diagnostic bytes */

#ifndef lint
static char Rcsid[] = "@(#) $Header: /tmp/cvs/psimail/lcd.c,v 1.1 1991/11/22 13:26:16 hans Exp $";
#endif
	
/*
 * $Header: /tmp/cvs/psimail/lcd.c,v 1.1 1991/11/22 13:26:16 hans Exp $
 *
 * This routine shamelessly stolen from ISODE. I think John Pavel must be
 * creditted with this routine.
 *
 * $Log: lcd.c,v $
 * Revision 1.1  1991/11/22 13:26:16  hans
 * Initial revision
 *
 * Revision 1.3  88/07/11  08:28:54  jpo
 * Some tidying up
 * 
 * Revision 1.2  87/11/04  15:07:19  jpo
 * Touch up
 * 
 * 
 * Revision 1.1  87/11/04  14:43:52  jpo
 * Initial revision
 * 
 *
 */
	
#include "psi_config.h"
	
#include <stdio.h>
#include <errno.h>
extern char *sys_errlist[];
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
#include <sysexits.h>
	
int
log_cause_and_diag(fp, fd)
	
	FILE *fp;
	int fd;
{
    X25_CAUSE_DIAG	diag;
	int 			rcode = EX_UNAVAILABLE;
	
    if (ioctl(fd, X25_RD_CAUSE_DIAG, &diag) < 0) {
		fprintf(fp, "X25_RD_CAUSE_DIAG failed (%s)\n", sys_errlist[errno]);
		return EX_OSERR;
    }

#ifdef DEBUG
    fprintf(fp, "Call cleared\n");
#endif
	
    if (diag.flags & (1 << RECV_DIAG)) {
		fprintf(fp, "call %s: 0x%2x 0x%2x\n",
				((diag.flags) & (1 << DIAG_TYPE)) ? "cleared"
				: "reset",
				diag.data[0], diag.data[1]);
		
		if ((diag.flags) & (1 << DIAG_TYPE)) /* cleared */
			switch(diag.data[0]) {
			  case 0x00:
				fprintf(fp, "clearing cause DTE Clearing\n");
				break;
				
			  case 0x01:
				fprintf(fp, "clearing cause Number Busy\n");
				rcode = EX_TEMPFAIL;
				break;
			  case 0x09:
				fprintf(fp, "clearing cause Out of Order\n");
				rcode = EX_TEMPFAIL;
				break;
			  case 0x11:
				fprintf(fp, "clearing cause Remote Procedure Error\n");
				break;
			  case 0x19:
				fprintf(fp, "clearing cause Reverse Charging not subscribed\n");
				break;
				
			  case 0x03:
				fprintf(fp, "clearing cause Invalid Facility Request\n");
				break;
			  case 0x0B:
				fprintf(fp, "clearing cause Access Barred\n");
				break;
			  case 0x13:
				fprintf(fp, "clearing cause Local Procedure Error\n");
				break;
				
			  case 0x05:
				fprintf(fp, "clearing cause Network Congestion\n");
				rcode = EX_TEMPFAIL;
				break;
			  case 0x0D:
				fprintf(fp, "clearing cause Not Obtainable\n");
				break;
				
			  case 0x21:
				fprintf(fp, "clearing cause DTE Incompatible Call\n");
				break;
				
			  case 0x29:
				fprintf(fp, "clearing cause Fast Select Acceptance not Subscribed\n");
				break;
				default:
				fprintf(fp, "clearing cause 0x2%x\n", diag.data[0]);
				break;
			}
		else /* reset */
			switch(diag.data[0]) {
			  case 0x00:
				fprintf(fp, "resetting cause DTE Reset\n");
				rcode = EX_TEMPFAIL;
				break;
			  case 0x01:
				fprintf(fp, "resetting cause Out of Order (PVC Only)\n");
				rcode = EX_TEMPFAIL;
				break;
			  case 0x03:
				fprintf(fp, "resetting cause Remote Procedure Error\n");
				break;
			  case 0x05:
				fprintf(fp, "resetting cause Local Procedure Error\n");
				rcode = EX_TEMPFAIL;
				break;
			  case 0x07:
				fprintf(fp, "resetting cause Network Congestion\n");
				rcode = EX_TEMPFAIL;
				break;
			  case 0x09:
				fprintf(fp, "resetting cause Remote DTE Operational (PVC Only)\n");
				break;
			  case 0x0F:
				fprintf(fp, "resetting cause Network Operational (PVC Only\n");
				break;
				default:
				fprintf(fp, "resetting cause 0x%2x\n", diag.data[0]);
				break;
			}
		
		/* The following may only be applicable to PSS in the UK */
		/* In any case, if someone is keen, they can stuff it all
		   into a text file and read it out */
		
        switch (diag.data[1]) {
		  case 0x00: 
			fprintf(fp, "diagnostic NO ADDITIONAL INFORMATION\n");
			break;
			
		  case 0x01: 
			fprintf(fp, "diagnostic INVALID P(S)\tRESET\n");
			break;
			
		  case 0x02: 
			fprintf(fp, "diagnostic INVALID P(R)\tRESET\n");
			break;
			
		  case 0x11: 
			fprintf(fp, "diagnostic PACKET TYPE INVALID FOR STATE r1\tRESTART\n");
			break;
			
		  case 0x12: 
			fprintf(fp, "diagnostic PACKET TYPE INVALID FOR STATE r2\tRESTART\n");
			break;
			
		  case 0x13: 
			fprintf(fp, "diagnostic PACKET TYPE INVALID FOR STATE r3\tRESTART\n");
			break;
			
		  case 0x14: 
			fprintf(fp, "diagnostic PACKET TYPE INVALID FOR STATE p1\tCLEAR\n");
			break;
			
		  case 0x15: 
			fprintf(fp, "diagnostic PACKET TYPE INVALID FOR STATE p2\tCLEAR\n");
			break;
			
		  case 0x16: 
			fprintf(fp, "diagnostic PACKET TYPE INVALID FOR STATE p3\tCLEAR\n");
			break;
			
		  case 0x17: 
			fprintf(fp, "diagnostic PACKET TYPE INVALID FOR STATE p4\tCLEAR\n");
			break;
			
		  case 0x18: 
			fprintf(fp, "diagnostic PACKET TYPE INVALID FOR STATE p5\tRESET\n");
			break;
			
		  case 0x19: 
			fprintf(fp, "diagnostic PACKET TYPE INVALID FOR STATE p6\tCLEAR\n");
			break;
			
		  case 0x1A: 
			fprintf(fp, "diagnostic PACKET TYPE INVALID FOR STATE p7\tCLEAR\n");
			break;
			
		  case 0x1B: 
			fprintf(fp, "diagnostic PACKET TYPE INVALID FOR STATE d1\tRESET\n");
			break;
			
		  case 0x1C: 
			fprintf(fp, "diagnostic PACKET TYPE INVALID FOR STATE d2\tRESET\n");
			break;
			
		  case 0x1D: 
			fprintf(fp, "diagnostic PACKET TYPE INVALID FOR STATE d3\tRESET\n");
			break;
			
		  case 0x20: 
			fprintf(fp, "diagnostic PACKET NOT ALLOWED\n");
			break;
			
		  case 0x21: 
			fprintf(fp, "diagnostic UNIDENTIFIABLE PACKET\n");
			break;
			
		  case 0x22: 
			fprintf(fp, "diagnostic CALL ON ONE-WAY LOGICAL CHANNEL\tCLEAR\n");
			break;
			
		  case 0x23: 
			fprintf(fp, "diagnostic INVALID PACKET TYPE ON PVC\tRESET\n");
			break;
			
		  case 0x24: 
			fprintf(fp, "diagnostic PACKET ON UNASSIGNED LCN\tCLEAR\n");
			break;
			
		  case 0x25: 
			fprintf(fp, "diagnostic REJECT NOT SUBSCRIBED TO\tRESET\n");
			break;
			
		  case 0x26: 
			fprintf(fp, "diagnostic PACKET TOO SHORT\tRESET\n");
			break;
			
		  case 0x27: 
			fprintf(fp, "diagnostic PACKET TOO LONG\tRESET\n");
			break;
			
		  case 0x28: 
			fprintf(fp, "diagnostic INVALID GFI\tCLEAR\n");
			break;
			
		  case 0x29: 
			fprintf(fp, "diagnostic RESTART WITH NON-ZERO BITS 5-16\n");
			break;
			
		  case 0x2A: 
			fprintf(fp, "diagnostic PACKET TYPE NOT COMPATIBLE WITH FACILITY\tCLEAR\n");
			break;
			
		  case 0x2B: 
			fprintf(fp, "diagnostic UNAUTHORISED INTERRUPT CONF\tRESET\n");
			break;
			
		  case 0x2C: 
			fprintf(fp, "diagnostic UNAUTHORISED INTERRUPT\tRESET\n");
			break;
			
		  case 0x31: 
			fprintf(fp, "diagnostic TIMER EXPIRED;  INCOMING CALL\n");
			break;
			
		  case 0x32: 
			fprintf(fp, "diagnostic TIMER EXPIRED;\tCLEAR INDICATION\n");
			break;
			
		  case 0x33: 
			fprintf(fp, "diagnostic TIMER EXPIRED;\tRESET INDICATION\n");
			break;
			
		  case 0x34: 
			fprintf(fp, "diagnostic TIMER EXPIRED;\tRESTART IND\n");
			break;
			
		  case 0x40: 
			fprintf(fp, "diagnostic UNSPECIFIED CALL SET-UP PROBLEM CLEAR\n");
			break;
			
		  case 0x41: 
			fprintf(fp, "diagnostic FACILITY CODE NOT ALLOWED\tCLEAR\n");
			break;
			
		  case 0x42: 
			fprintf(fp, "diagnostic FACILITY PARAMETER NOT ALLOWED\tCLEAR\n");
			break;
			
		  case 0x43: 
			fprintf(fp, "diagnostic INVALID CALLED ADDRESS\tCLEAR\n");
			break;
			
		  case 0x44: 
			fprintf(fp, "diagnostic INVALID CALLING ADDRESS\tCLEAR\n");
			break;
			
		  case 0x90: 
			fprintf(fp, "diagnostic DTE/DCE CONGESTION\tRESET\n");
			break;
			
		  case 0x91: 
			fprintf(fp, "diagnostic RECEIVED FAST SELECT CLEAR REQUEST\n");
			break;
			
		  case 0x92: 
			fprintf(fp, "diagnostic LINE RESTARTING BY INMC COMMAND\tRESTART\n");
			break;
			
		  case 0xA0: 
			fprintf(fp, "diagnostic NON-ZERO RESET CAUSE FROM DTE\tRESET\n");
			break;
			
		  case 0xA1: 
			fprintf(fp, "diagnostic DATA PACKET TOO LONG\tRESET\n");
			break;
			
		  case 0xA2: 
			fprintf(fp, "diagnostic INTERRUPT PACKET TOO LONG\tRESET\n");
			break;
			
		  case 0xA3: 
			fprintf(fp, "diagnostic INT PACKET TOO SHORT; NO USER DATA\tRESET\n");
			break;
			
		  case 0xA4: 
			fprintf(fp, "diagnostic INT CONFIRMATION PACKET TOO LONG\tRESET\n");
			break;
			
		  case 0xA5: 
			fprintf(fp, "diagnostic RR PACKET TOO LONG\tRESET\n");
			break;
			
		  case 0xA6: 
			fprintf(fp, "diagnostic RNR PACKET TOO LONG\tRESET\n");
			break;
			
		  case 0xA7: 
			fprintf(fp, "diagnostic RESET PACKET TOO LONG\tRESET\n");
			break;
			
		  case 0xA8: 
			fprintf(fp, "diagnostic RESET CONF PACKET TOO LONG\tRESET\n");
			break;
			
		  case 0xA9: 
			fprintf(fp, "diagnostic INVALID `Q' BIT IN DATA PACKET\tRESET\n");
			break;
			
		  case 0xAA: 
			fprintf(fp, "diagnostic PACKET WINDOW RANGE EXCEEDED\tRESET\n");
			break;
			
		  case 0xAB: 
			fprintf(fp, "diagnostic UNABLE TO TRANSMIT PACKET\tRESET\n");
			break;
			
		  case 0xAC: 
			fprintf(fp, "diagnostic `Q' BIT SET IN NON-DATA PACKET\tRESET\n");
			break;
			
		  case 0xAD: 
			fprintf(fp, "diagnostic OUTSTANDING PACKET COUNT LESS THAN ZERO\tRESET\n");
			break;
			
		  case 0xAE: 
			fprintf(fp, "diagnostic RETRANSMISSION ERROR\tRESET\n");
			break;
			
		  case 0xAF: 
			fprintf(fp, "diagnostic RESET PACKET TOO SHORT (NO CAUSE)\tRESET\n");
			break;
			
		  case 0xB0: 
			fprintf(fp, "diagnostic REJECT PACKET TOO LONG\tRESET\n");
			break;
			
		  case 0xB1: 
			fprintf(fp, "diagnostic INVALID 1D PACKET\tRESET\n");
			break;
			
		  case 0xB2: 
			fprintf(fp, "diagnostic UNSUCCESSFUL RECONNECTION RESNC\tCLEAR\n");
			break;
			
		  case 0xB3: 
			fprintf(fp, "diagnostic NON-RECONNECT CALL IN STATE C1\tCLEAR\n");
			break;
			
		  case 0xB4: 
			fprintf(fp, "diagnostic SECOND 1D PACKET FROM DTE\tCLEAR\n");
			break;
			
		  case 0xB5: 
			fprintf(fp, "diagnostic BAD DATA TRANSFER STATE IN RECONNECT\tCLEAR\n");
			break;
			
		  case 0xB6: 
			fprintf(fp, "diagnostic PACKET FORMAT INVALID\tCLEAR\n");
			break;
			
		  case 0xB7: 
			fprintf(fp, "diagnostic FACILITY BYTE COUNT TOO LARGE\tCLEAR\n");
			break;
			
		  case 0xB8: 
			fprintf(fp, "diagnostic INVALID PACKET DETECTED\tCLEAR\n");
			break;
			
		  case 0xB9: 
			fprintf(fp, "diagnostic FACILITY/UTILITY FIELD BYTE COUNT > 63\tCLEAR\n");
			break;
			
		  case 0xBA: 
			fprintf(fp, "diagnostic OUTGOING CALLS BARRED\tCLEAR\n");
			break;
			
		  case 0xBB: 
			fprintf(fp, "diagnostic INCOMING CALLS BARRED\tCLEAR\n");
			break;
			
		  case 0xBC: 
			fprintf(fp, "diagnostic CLEARING OF PVC\tCLEAR\n");
			break;
			
		  case 0xBD: 
			fprintf(fp, "diagnostic CALLED ADDRESS TOO LONG\tCLEAR\n");
			break;
			
		  case 0xBE: 
			fprintf(fp, "diagnostic CALLED ADDRESS TOO SHORT\tCLEAR\n");
			break;
			
		  case 0xBF: 
			fprintf(fp, "diagnostic CALLING ADDRESS TOO LONG\tCLEAR\n");
			break;
			
		  case 0xC0: 
			fprintf(fp, "diagnostic CALLING ADDRESS TOO SHORT\tCLEAR\n");
			break;
			
		  case 0xC1: 
			fprintf(fp, "diagnostic BCD ERROR IN CALL ADDRESS\tCLEAR\n");
			break;
			
		  case 0xC2: 
			fprintf(fp, "diagnostic BCD ERROR IN CALLING ADDRESS\tCLEAR\n");
			break;
			
		  case 0xC3: 
			fprintf(fp, "diagnostic USER DATA FIELD TOO LONG\tCLEAR\n");
			break;
			
		  case 0xC4: 
			fprintf(fp, "diagnostic NO BUFFER AVAILABLE\tCLEAR\n");
			break;
			
		  case 0xC5: 
			fprintf(fp, "diagnostic LOCAL DTE IS NOT ENHANCED\tCLEAR\n");
			break;
			
		  case 0xC6: 
			fprintf(fp, "diagnostic FACILITY NEGOTIATION INVALID\tCLEAR\n");
			break;
			
		  case 0xC7: 
			fprintf(fp, "diagnostic MANDATORY UTILITY NOT INPUT\tCLEAR\n");
			break;
			
		  case 0xC8: 
			fprintf(fp, "diagnostic BUFFER NO AVAILABLE FOR TNIC\tCLEAR\n");
			break;
			
		  case 0xC9: 
			fprintf(fp, "diagnostic OVERFLOW OF TNIC IN BUFFER\tCLEAR\n");
			break;
			
		  case 0xCA: 
			fprintf(fp, "diagnostic DTE LINECONGESTED\tCLEAR\n");
			break;
			
		  case 0xCB: 
			fprintf(fp, "diagnostic TABLE ERROR IN PACKET PROCEDURES\n");
			break;
			
		  case 0xCC: 
			fprintf(fp, "diagnostic INSERT TABLE OVERFLOW\n");
			break;
			
		  case 0xCD: 
			fprintf(fp, "diagnostic DELETE TABLE OVERFLOW\n");
			break;
			
		  case 0xD0: 
			fprintf(fp, "diagnostic TRUNK LINE RESTART\tRESTART\n");
			break;
			
		  case 0xD1: 
			fprintf(fp, "diagnostic INVALID EVENT IN STATE p2\n");
			break;
			
		  case 0xD2: 
			fprintf(fp, "diagnostic INVALID EVENT IN STATE p3\n");
			break;
			
		  case 0xD3: 
			fprintf(fp, "diagnostic INVALID 1D EVENT IN STATE d1\n");
			break;
			
		  case 0xD4: 
			fprintf(fp, "diagnostic CALL COLLISION ON TRUNK LINE\n");
			break;
			
		  case 0xD5: 
			fprintf(fp, "diagnostic NO BUFFER AVAILABLE\n");
			break;
			
		  case 0xD6: 
			fprintf(fp, "diagnostic CALL COLLISION ON DTE LINE\n");
			break;
			
		  case 0xD7: 
			fprintf(fp, "diagnostic DTE RESTART\n");
			break;
			
		  case 0xD8: 
			fprintf(fp, "diagnostic CALL REQUEST TO TRUNK LINE TIMEOUT\n");
			break;
			
		  case 0xD9: 
			fprintf(fp, "diagnostic RECONNECT SET-UP TIMED OUT\n");
			break;
			
		  case 0xDA: 
			fprintf(fp, "diagnostic INVALID OUTPUT SIDE STATE\n");
			break;
			
		  case 0xDB: 
			fprintf(fp, "diagnostic ERROR DETECTED IN BLINK PACKET QUEUE PROCEDURE\n");
			break;
			
		  case 0xDC: 
			fprintf(fp, "diagnostic RESET INDICATION RETRANSMISSION COUNT EXPIRED\n");
			break;
			
		  case 0xDD: 
			fprintf(fp, "diagnostic INVALID OUTPUT SIDE STATE\n");
			break;
			
		  case 0xDE: 
			fprintf(fp, "diagnostic BLIND BUFFER QUEUE OVERFLOW IN STATE d4\n");
			break;
			
		  case 0xDF: 
			fprintf(fp, "diagnostic BLIND BUFFER QUEUE OVERFLOW IN STATE c1\n");
			break;
			
		  case 0xE0: 
			fprintf(fp, "diagnostic BLIND BUFFER QUEUE OVERFLOW IN STATE c2\n");
			break;
			
		  case 0xE1: 
			fprintf(fp, "diagnostic CLEAR PACKET BYTE COUNT TOO LARGE OR TOO SMALL\n");
			break;
			
		  case 0xE2: 
			fprintf(fp, "diagnostic NON-ZERO\tCLEAR CAUSE\n");
			break;
			
		  case 0xE3: 
			fprintf(fp, "diagnostic CLEAR CONF PACKET BYTE COUNT TOO SMALL OR TOO LARGE\n");
			break;
			
		  case 0xE4: 
			fprintf(fp, "diagnostic CALL COLLISIO\n");
			break;
			
		  case 0xE5: 
			fprintf(fp, "diagnostic INVALID TP LOAD REQUEST CALL PKT\n");
			break;
			
		  case 0xE6: 
			fprintf(fp, "diagnostic MAXIMUM HOPCOUNT EXCEEDED\n");
			break;
			
		  case 0xE7: 
			fprintf(fp, "diagnostic ROUTING LOOP DETECTED\n");
			break;
			
		  case 0xE8: 
			fprintf(fp, "diagnostic PVC CALL REQUEST FAILURE\n");
			break;
			
		  case 0xE9: 
			fprintf(fp, "diagnostic RECONNECT CALL REQUEST FAILED\n");
			break;
			
		  case 0xEA: 
			fprintf(fp, "diagnostic NO LC AVAILABLE ON OUTPUT SIDE\n");
			break;
			
		  case 0xEB: 
			fprintf(fp, "diagnostic NO BUFFER AVAILABLE\n");
			break;
			
		  case 0xEC: 
			fprintf(fp, "diagnostic CALL REDIRECTION CLEAR\n");
			break;
			
		  case 0xED: 
			fprintf(fp, "diagnostic NO PATH ROUTE CALL\n");
			break;
			
		  case 0xEE: 
			fprintf(fp, "diagnostic CALL ROUTED TO DTE LINE\n");
			break;
			
		  case 0xEF: 
			fprintf(fp, "diagnostic CALL CANNOT BE REROUTED\n");
			break;
			
		  case 0xF0: 
			fprintf(fp, "diagnostic ADDRESS NOT IN ROUTING TABLES\n");
			break;
			
		  case 0xF1: 
			fprintf(fp, "diagnostic ROUTING TABLE CHANGE DURING CALL ROUTING\n");
			break;
			
		  case 0xF2: 
			fprintf(fp, "diagnostic NO LC AVAILABLE ON FAKE TRUNK\n");
			break;
			
		  case 0xF3: 
			fprintf(fp, "diagnostic REMOTE DTE DOWN ON A PVC\n");
			break;
			
		  case 0xF4: 
			fprintf(fp, "diagnostic INVALID EVENT DETECTED\n");
			break;
			
		  case 0xF5: 
			fprintf(fp, "diagnostic INVALID PACKET RECEIVED; STATE d4\n");
			break;
			
		  case 0xF6: 
			fprintf(fp, "diagnostic INVALID PACKET RECEIVED; STATE d5\n");
			break;
			
		  case 0xF7: 
			fprintf(fp, "diagnostic INVALID PACKET RECEIVED; STATE p8\n");
			break;
			
		  case 0xF8: 
			fprintf(fp, "diagnostic INTERNAL PROCESSING FAILURE\n");
			break;
			
		  case 0xF9: 
			fprintf(fp, "diagnostic INVALID RESTART INDICATIO\n");
			break;
			
		  case 0xFA: 
			fprintf(fp, "diagnostic LINE STATUS CHANGE IN STATE r4\n");
			break;
			
		  case 0xFB: 
			fprintf(fp, "diagnostic INVALID PACKET RECEIVED; STATE r4\n");
			break;
			
		  case 0xFC: 
			fprintf(fp, "diagnostic INVALID PACKET RECEIVED; STATE r3\n");
			break;
			
		  case 0xFD: 
			fprintf(fp, "diagnostic LINE STATUS CHANGE IN STATE r2\n");
			break;
			
		  case 0xFE: 
			fprintf(fp, "diagnostic LINE STATUS CHANGE IN STATE r1\n");
			break;
			
		  case 0xFF: 
			fprintf(fp, "diagnostic LINE STATUS CHANGE IN STATE r0\n");
			break;
			
			default:
			fprintf(fp, "diagnostic: 0x%2x\n", diag.data[1]);
			break;
        }
    }
    else /* Not RECV_DIAG */
        if (diag.flags)
			fprintf(fp, "diag flags: 0x%2x\n", diag.flags);

	return rcode;
}

