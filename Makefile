PREFIX ?= /usr
BINDIR ?= bin
MANDIR ?= share/man/man1
LIBDIR ?= lib/sendip

OS := $(shell uname -s)
MACH ?= 64

INSTALL_SunOS = ginstall
INSTALL_Linux = install

INSTALL ?= $(INSTALL_$(OS))

# If this is set to 'cc', *_cc flags (Sun studio compiler) will be used.
# If set to 'gcc', the corresponding GNU C flags (*_gcc) will be used.
# For all others one needs to add corresponding rules.
CC ?= gcc
OPTIMZE_cc ?= -xO3
OPTIMZE_gcc ?= -O3
OPTIMZE ?= -g $(OPTIMZE_$(CC))

CFLAGS_cc = -xcode=pic32
CFLAGS_cc += -errtags -erroff=%none,E_UNRECOGNIZED_PRAGMA_IGNORED,E_ATTRIBUTE_UNKNOWN,E_NONPORTABLE_BIT_FIELD_TYPE -errwarn=%all -D_XOPEN_SOURCE=600 -D__EXTENSIONS__=1
#CFLAGS_cc += -pedantic -v
CFLAGS_gcc = -fPIC -fsigned-char -pipe -Wno-unknown-pragmas -Wno-unused-result
CFLAGS_gcc += -fdiagnostics-show-option -Wall -Werror
#CFLAGS_gcc += -pedantic -Wpointer-arith -Wwrite-strings -Wstrict-prototypes -Wnested-externs -Winline -Wextra -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs -Wredundant-decls -Wshadow -Wundef -Wunused -Wno-variadic-macros -Wno-parentheses -Wcast-align -Wcast-qual

CFLAGS ?=  -m$(MACH) $(CFLAGS_$(CC)) $(OPTIMZE)
CFLAGS += -DSENDIP_LIBS=\"$(PREFIX)/$(LIBDIR)\"

LIBS_SunOS = -lsocket -lnsl -lm
LIBS_Linux = -ldl -lm -lbsd
LIBS ?= $(LIBS_$(OS))

SHARED_cc := -G
SHARED_gcc := -shared
LDFLAGS_cc := -zdefs -Bdirect -zdiscard-unused=dependencies $(LIBS)
LDFLAGS_gcc := -zdefs -Wl,--as-needed $(LIBS)
SONAME_OPT_cc := -h
SONAME_OPT_gcc := -Wl,-soname,
RPATH_OPT_cc := -R
RPATH_OPT_gcc := -Wl,-rpath=

LDFLAGS ?= -m$(MACH) $(LDFLAGS_$(CC))
SHARED := $(SHARED_$(CC))
SONAME_OPT := $(SONAME_OPT_$(CC))
RPATH_OPT := $(RPATH_OPT_$(CC))

LIBCFLAGS = $(CFLAGS) $(SHARED) $(LDFLAGS) -lc

PROGS= sendip
DYNLIBEXT= .so
DYNLIB_MAJOR= 1
DYNLIB_MINOR= 0

LIBRARY= sendip
SOBN= lib$(LIBRARY)$(DYNLIBEXT)
SONAME= $(SOBN).$(DYNLIB_MAJOR)
DYNLIB= $(SONAME).$(DYNLIB_MINOR)

LIBSRCS= csum.c compact.c fargs.c protoname.c headers.c parseargs.c \
	c_origin.c modload.c
LIBOBJS= $(LIBSRCS:%.c=%.o)

PROGSRCS = sendip.c gnugetopt.c
PROGOBJS = $(PROGSRCS:%.c=%.o) 

BASEPROTOS= ipv4.so ipv6.so
IPPROTOS= icmp.so tcp.so udp.so
UDPPROTOS= rip.so ripng.so ntp.so
TCPPROTOS= bgp.so
# contributions by Mark Carson (NIST - "IPv6 Tools and Test Materials")
CRYPTOS= xorauth.so xorcrypto.so
MECPROTOS= ah.so dest.so esp.so frag.so gre.so hop.so route.so sctp.so wesp.so

PROTOS= $(BASEPROTOS) $(IPPROTOS) $(UDPPROTOS) $(TCPPROTOS) $(MECPROTOS)

PSEUDO_HEADER = dest.h xorauth.h xorcrypto.h

all:	$(PROGS) $(PROTOS) $(CRYPTOS) sendip.spec
lib:	$(DYNLIB)

$(PROGS):	LDFLAGS += $(RPATH_OPT)\$$ORIGIN/../$(LIBDIR)
$(PROTOS) $(CRYPTOS):	LDFLAGS += $(RPATH_OPT)\$$ORIGIN
hop.so:		CFLAGS += -DHOP_OPT
dest.so:	CFLAGS += -DDEST_OPT

%.so: %.c %.h $(DYNLIB)
	$(CC) -o $@ $< $(DYNLIB) $(LIBCFLAGS)

$(DYNLIB): $(LIBOBJS)
	$(CC) -o $@ $(SHARED) $(SONAME_OPT)$(SONAME) $(LIBOBJS) $(LIBCFLAGS)

$(PROGS):	$(PROGOBJS) $(DYNLIB)
	$(CC) -o $@ $(PROGOBJS) $(DYNLIB) $(LDFLAGS)

# a kludge to keep rules straight
dest.c:
	ln -s hop.c $@

$(PSEUDO_HEADER):
	touch $@

sendip.spec:	sendip.spec.in VERSION
	printf '%%define ver ' >sendip.spec
	cat VERSION sendip.spec.in >>sendip.spec


.PHONY:	clean distclean install depend help

# for maintainers to get _all_ deps wrt. source headers properly honored
DEPENDFILE := makefile.dep

depend: $(DEPENDFILE)

$(DEPENDFILE): *.c *.h
	makedepend -f - -Y/usr/include -DHOP_OPT *.c 2>/dev/null | \
		sed -e 's@/usr/include/[^ ]*@@g' -e '/: *$$/ d' >makefile.dep

clean:
	rm -f *.o *~ *.so $(PROTOS) $(CRYPTOS) $(SONAME)* $(PROGS) \
		core gmon.out a.out

distclean: clean
	rm -f sendip.spec dest.c $(PSEUDO_HEADER) $(DEPENDFILE) *.rej *.orig

install:	$(SUBDIRS) all
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/$(LIBDIR)
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/$(BINDIR)
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/$(MANDIR)
	$(INSTALL) -m 755 $(PROGS) $(DESTDIR)$(PREFIX)/$(BINDIR)
	$(INSTALL) -m 755 $(PROTOS) $(DYNLIB) $(DESTDIR)$(PREFIX)/$(LIBDIR)
	$(INSTALL) -m 755 $(CRYPTOS) $(DYNLIB) $(DESTDIR)$(PREFIX)/$(LIBDIR)
	$(INSTALL) -m 644 sendip.1 $(DESTDIR)$(PREFIX)/$(MANDIR)
	ln -sf $(DYNLIB) $(DESTDIR)$(PREFIX)/$(LIBDIR)/$(SONAME)
	ln -sf $(DYNLIB) $(DESTDIR)$(PREFIX)/$(LIBDIR)/$(SOBN)

help: $(PROGS) $(PROTOS)
	ln -sf $(DYNLIB) $(SONAME)
	LD_LIBRARY_PATH_$(MACH)=. LD_LIBRARY_PATH=. \
		./sendip -p $(PROTOS:%.so=% -p) tcp -h


-include $(DEPENDFILE)
