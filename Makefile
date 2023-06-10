SHELL=/bin/sh
# CFLAGS=-DUSE_X25LIB -g
CFLAGS = -DSUNLINK6 # -g
LDFLAGS= # -g -Bstatic
# LIBS=/develop/lib/Pub/O_sx386/libx25.a /develop/lib/Pub/O_sx386/libml.a

all:	psi_incom psi_out

install:
		cp psi_incom psi_out /usr/lib/x25/psi

psi_incom:	psi_incom.o readcf.o lcd.o
		cc $(LDFLAGS) -o psi_incom psi_incom.o readcf.o lcd.o $(LIBS)

psi_out:	psi_out.o readcf.o lcd.o
		cc $(LDFLAGS) -o psi_out psi_out.o readcf.o lcd.o $(LIBS)

depend:
		makedepend $(CFLAGS) psi_incom.c psi_out.c readcf.c

clean:
		rm -f *.o psi_out psi_incom core

# DO NOT DELETE THIS LINE -- make depend depends on it.

psi_incom.o: psi_config.h /usr/include/stdio.h /usr/include/sys/types.h
psi_incom.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
psi_incom.o: /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
psi_incom.o: /usr/include/sys/ttydev.h /usr/include/sys/ttold.h
psi_incom.o: /usr/include/sys/ioccom.h /usr/include/sys/ttycom.h
psi_incom.o: /usr/include/sys/filio.h /usr/include/sys/sockio.h
psi_incom.o: /usr/include/sys/param.h /usr/include/machine/param.h
psi_incom.o: /usr/include/sys/signal.h /usr/include/vm/faultcode.h
psi_incom.o: /usr/include/sys/systm.h /usr/include/sys/kmem_alloc.h
psi_incom.o: /usr/include/sys/mbuf.h /usr/include/sys/socket.h
psi_incom.o: /usr/include/sys/protosw.h /usr/include/sys/domain.h
psi_incom.o: /usr/include/sys/socketvar.h /usr/include/sys/errno.h
psi_incom.o: /usr/include/net/if.h /usr/include/sundev/syncstat.h
psi_incom.o: /usr/include/netx25/x25_pk.h /usr/include/netx25/x25_ctl.h
psi_incom.o: /usr/include/netx25/x25_ioctl.h /usr/include/ctype.h
psi_incom.o: /usr/include/errno.h /usr/include/sys/utsname.h
psi_out.o: psi_config.h /usr/include/stdio.h /usr/include/sysexits.h
psi_out.o: /usr/include/fcntl.h /usr/include/sys/fcntlcom.h
psi_out.o: /usr/include/sys/stdtypes.h /usr/include/sys/stat.h
psi_out.o: /usr/include/sys/types.h /usr/include/sys/sysmacros.h
psi_out.o: /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
psi_out.o: /usr/include/sys/ttydev.h /usr/include/sys/ttold.h
psi_out.o: /usr/include/sys/ioccom.h /usr/include/sys/ttycom.h
psi_out.o: /usr/include/sys/filio.h /usr/include/sys/sockio.h
psi_out.o: /usr/include/sys/param.h /usr/include/machine/param.h
psi_out.o: /usr/include/sys/signal.h /usr/include/vm/faultcode.h
psi_out.o: /usr/include/sys/systm.h /usr/include/sys/kmem_alloc.h
psi_out.o: /usr/include/sys/mbuf.h /usr/include/sys/socket.h
psi_out.o: /usr/include/sys/protosw.h /usr/include/sys/domain.h
psi_out.o: /usr/include/sys/socketvar.h /usr/include/sys/errno.h
psi_out.o: /usr/include/net/if.h /usr/include/sundev/syncstat.h
psi_out.o: /usr/include/netx25/x25_pk.h /usr/include/netx25/x25_ctl.h
psi_out.o: /usr/include/netx25/x25_ioctl.h
readcf.o: psi_config.h /usr/include/stdio.h /usr/include/string.h
readcf.o: /usr/include/sys/stdtypes.h
