# SendIP

Sendip is command line tool to send arbitrary IP packets. It has a large number
of command line options to specify the content of every header of a NTP, BGP,
RIP, RIPng, TCP, UDP, ICMP, or raw IPv4 or IPv6 packet.  It also allows any 
data to be added to the packet.

## Repo

This repo is a fork of **Mike Ricketts**'
[SendIP 2.5, 29/07/2003](http://www.earth.li/projectpurple/progs/sendip.html)
(now also available via [Github](https://github.com/rickettm/SendIP))
and contains major enhancements implemented by Mark Carson which in turn can be
found via the [NIST SendIP](https://www-x.antd.nist.gov/ipv6/sendip.html) web
page as well as many other contributors.

For now repo's main purpose is actually to collect and apply
patches/improvements found in the wild and make it available to the world.

If you need some new features (or bug fixes), please feel free to create an
issue via https://github.com/jelmd/sendip/issues .


### Versioning

Starting with version 3.0.0 sendip follows the basic idea of semantic
versioning, but having the real world in mind. Therefore __official releases
have always THREE numbers__ (A.B.C), not more and not less!

In general we use A.B.C.D.E, whereby trailing zeros get dropped, if the version
string has more than 3 numbers. The .D.E part might be used for unreleased or
current builds of the master, other "work in progress" branches or releases
from different vendors. However, __wrt. D the number 0 is reserved__ for
us (sendip maintainers).  All other numbers can be used as needed by
people/organizations/etc., which build their own packages and thus have the
chance to supersede upstream releases without breaking semantic versioning.
For completeness only: For now _E_ gets used incrementally, but might
be used in future to express e.g.  nightly, alpha, beta, RC, or FCS builts.
In this case another number pair (_.F.G_) might be used to tag related
versions, but _non-digits_ except the dot (_._) will _never_ be used to
produce a valid version string!
