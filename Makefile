PREFIX ?= /usr
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man/man1
LIBDIR ?= $(PREFIX)/lib/sendip

OS := $(shell uname -s)

INSTALL_SunOS = ginstall
INSTALL_Linux = install

INSTALL ?= $(INSTALL_$(OS))

CC ?= gcc

MACH ?= 64
CFLAGS_cc = -xcode=pic32
CFLAGS_gcc = -fPIC -fsigned-char -pipe -Wall -Wpointer-arith -Wwrite-strings \
	-Wstrict-prototypes -Wnested-externs -Winline -Werror -g -Wcast-align
CFLAGS ?=  -m$(MACH) $(CFLAGS_$(CC))
CFLAGS += -DSENDIP_LIBS=\"$(LIBDIR)\"

LIBS_SunOS = -lsocket -lnsl -lm
LIBS_Linux = -ldl -lm
LIBS ?= $(LIBS_$(OS))

LDFLAGS_cc = -zdefs -Bdirect -zdiscard-unused=dependencies $(LIBS)
LDFLAGS_gcc = -zdefs -Wl,--as-needed $(LIBS)
LDFLAGS ?= -g -m$(MACH) $(LDFLAGS_$(CC))

LIBCFLAGS = -shared $(LDFLAGS) -lc $(CFLAGS)

PROGS= sendip
BASEPROTOS= ipv4.so ipv6.so
IPPROTOS= icmp.so tcp.so udp.so
UDPPROTOS= rip.so ripng.so ntp.so
TCPPROTOS= bgp.so
PROTOS= $(BASEPROTOS) $(IPPROTOS) $(UDPPROTOS) $(TCPPROTOS)
GLOBALOBJS= csum.o compact.o

all:	$(GLOBALOBJS) sendip $(PROTOS) sendip.spec

sendip:	sendip.o	gnugetopt.o compact.o
	$(CC) -o $@ $+ $(LDFLAGS)

sendip.spec:	sendip.spec.in VERSION
			printf '%%define ver ' >sendip.spec
			cat VERSION sendip.spec.in >>sendip.spec

%.so: %.c $(GLOBALOBJS)
			$(CC) -o $@ $+ $(LIBCFLAGS)

.PHONY:	clean distclean install

clean:
			rm -f *.o *~ *.so $(PROTOS) $(PROGS) core gmon.out

distclean: clean
			rm -f sendip.spec

install:		all
			$(INSTALL) -d $(DESTDIR)$(LIBDIR)
			$(INSTALL) -d $(DESTDIR)$(BINDIR)
			$(INSTALL) -d $(DESTDIR)$(MANDIR)
			$(INSTALL) -m 755 $(PROGS) $(DESTDIR)$(BINDIR)
			$(INSTALL) -m 755 $(PROTOS) $(DESTDIR)$(LIBDIR)
			$(INSTALL) -m 644 sendip.1 $(DESTDIR)$(MANDIR)
