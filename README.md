# 💡 What is RSPLIB?

[Reliable Server Pooling&nbsp;(RSerPool)](https://www.nntb.no/~dreibh/rserpool/#Description) is the new IETF framework for server pool management and session failover handling. In particular, it can be used for realising highly available services and load distribution. RSPLIB is the reference implementation of RSerPool. It includes:

* The library librsplib, which is the RSerPool implementation itself;
* The library libcpprspserver, which is a C++ wrapper library to easily write server applications based on librsplib;
* A collection of server (pool element) and client (pool user) examples.


# 🗃️ First Steps

## Ensure that multicast is working!

You need a configured network interface with:

* at least a private address (192.168.*x*.*y*; 10.*a*.*b*.*c*; 172.16.*n*.*m* - 172.31.*i*.*j*)
* having the multicast flag set (e.g. ```sudo ifconfig <dev> multicast```)

In a typical network setup, this should already be configured.

Ensure that your firewall settings allow UDP packets to/from the registrar (ASAP Announce/ENRP Presence), as well as ASAP/ENRP traffic over SCTP.

## Start at least one registrar

```
rspregistrar
```

See [Registrar](#registrar) for registrar parameters.

## Start at least one pool element

```
rspserver -echo
```

You can start multiple pool elements; they may also run on different hosts, of course. If it complains about finding no registrar, check the multicast settings!

## Start a pool user

```
rspterminal
```

If it complains about finding no registrar, check the multicast settings!
Now, you have to manually enter some text lines on standard input.

## Testing a failover

If everything works, you can test RSerPool functionality by stopping the pool element and watching the failover.

## Monitor the component status

You can monitor the status of each component using the Component Status Protocol monitor "cspmonitor". Simply start it by ```cspmonitor```. It will listen for status messages sent via UDP on port&nbsp;2960. The components (registrar, rspserver, etc.) accept the command line arguments ```-cspserver=<server>:<port>``` and ```-cspinterval=<milliseconds>```. For example, if you want a status update every 300&nbsp;ms and your CSP client is listening on port 2960 of host 192.168.11.22, use the arguments
```
... -cspserver=192.168.11.22:2960 -cspinterval=300
```

Note: You *must* specify address **and** interval, otherwise no messages are sent.

## Run Wireshark

You can use [Wireshark](https://www.wireshark.org) to observe the RSerPool and demo protocol traffic. Coloring rules and filters can be found in the directory <tt>[rsplib/src/wireshark](https://github.com/dreibh/rsplib/tree/master/src/wireshark)</tt>. Simply copy <tt>[colorfilters](https://github.com/dreibh/rsplib/blob/master/src/wireshark/colorfilters)</tt>, <tt>[dfilters](https://github.com/dreibh/rsplib/blob/master/src/wireshark/dfilters)</tt> and optionally <tt>[preferences](https://github.com/dreibh/rsplib/blob/master/src/wireshark/preferences)</tt> to <tt>$HOME/.wireshark</tt>. Dissectors for the RSerPool and application protocols are already included in recent Wireshark distributions!


# 📚 Pool Elements (Servers)

All demo servers can be started using the rspserver program. It takes a set of common parameters as well as some service-specific arguments. These parameters are explained in the following.

Notes:

* For most of the provided services, the latest version of [Wireshark](https://www.wireshark.org) already includes the packets dissectors!
* The manpages of the programs provide a detailed description of the options!


## Common Parameters

* ```-loglevel=0-9```: Sets the logging verbosity from 0 (none) to 9 (very verbose).
* ```-logcolor=on|off```: Turns ANSI colorization of the logging output on or off.
* ```-logfile=<filename>```: Write logging output to a file (default is stdout).
* ```-poolhandle=<poolhandle>```: Sets the PH to a non-default value; otherwise, the default setting will be the service-specific default (see below).
* ```-registrar=<address>:<port>```: Adds a static PR entry into the Registrar Table. It is possible to add multiple entries.
* ```-asapannounce=<address>:<port>```: Sets the multicast address and port the ASAP instance listens for ASAP Server Announces on.
* ```-rereginterval=<milliseconds>```: Sets the PE's re-registration interval (in milliseconds).
* ```-runtime=<seconds>```: After the configured amount of seconds, the service is shut down.
* ```-quiet```: Do not print startup and shutdown messages.
* ```-policy=<policy>```: Sets the pool policy and its parameters:
  - ```Random```
  - ```WeightedRandom:<weight>```
  - ```RoundRobin```
  - ```WeightedRoundRobin:<weight>```
  - ```LeastUsed```
  - ```LeastUsedDegradation:<degradation>```
  - ...


## Echo Service

```-echo```: Selects Echo service. The default PH will be "EchoPool".


## Discard Service

```-discard```: Selects Discard service. The default PH will be "DiscardPool".


## Daytime Service

```-daytime```: Selects Daytime service. The default PH will be "DaytimePool".


## Character Generator Service

```-chargen```: Selects Character Generator service. The default PH will be "ChargenPool".


## Ping Pong Service

```-pingpong```: Selects Ping Pong service. The default PH will be "PingPongPool".

The Ping Pong service provides further options:

* ```-pppfailureafter=<number_of_messages>```: After the set number of messages, the server will terminate the connection in order to test failovers.
* ```-pppmaxthreads=<threads>```: Sets the maximum number of simultaneous sessions.


## Fractal Generator Service

```-fractal```: Selects the Fractal Generator service. The default PH will be "FractalGeneratorPool".

The Fractal Generator service provides further options:

* ```-fgpcookiemaxtime=<milliseconds>```: Send cookie after given number of milliseconds.
* ```-fgpcookiemaxpackets=<numner_of_messages>```: Send cookie after given number of Data messages.
* ```-fgptransmittimeout=<milliseconds>```: Set transmit timeout in milliseconds (timeout for rsp_sendmsg()).
* ```-fgptestmode```: Generate simple test pattern instead of calculating a fractal graphics (useful to conserve CPU power).
* ```-fgpfailureafter=<number_of_messages>```: After the set number of Data messages, the server will terminate the connection in order to test failovers.
* ```-fgpmaxthreads=<threads>```: Sets the maximum number of simultaneous sessions.



# 📚 Pool Users (Clients)

## Common Parameters

-loglevel=0-9: Sets the logging verbosity from 0 (none) to 9 (very verbose).

-logcolor=on|off: Turns ANSI colorization of the logging output on or off.

-logfile=filename: Write logging output to a file (default is stdout).

-poolhandle=PH: Sets the PH.


## Terminal (e.g. for Echo, Discard, Daytime and Chargen services)

Start with ./rspterminal; the default PH is EchoPool.


## Fractal Client (for the Fractal Generator service)

./fractalpooluser

-configdir=Directory: Sets a directory to look for FGP config files. From all FGP files
                      (pattern: *.fgp) in this directory, random files are selected for the
                      calculation of requests.

-threads=Count:       Sets the number of parallel sessions for the calculation
                      of an image.

-caption=Title:       Sets the window title.


## Ping Pong Client (for the Fractal Generator service)

./pingpong

-interval=milliseconds: Sets the Ping interval.



# 📚 Registrar

Start the registrar with:

./rspregistrar



## Common Parameters

-loglevel=0-9: Sets the logging verbosity from 0 (none) to 9 (very verbose).

-logcolor=on|off: Turns ANSI colorization of the logging output on or off.

-logfile=filename: Write logging output to a file (default is stdout).

-peer=Address:Port: Adds a static PR entry into the Peer List.
                    It is possible to add multiple entries.


## ASAP Parameters

-asap=auto|address:port{,address}: Sets the ASAP endpoint address(es). Use "auto"
                                   to automatically set it (default).
                                   Examples: -asap=auto
                                             -asap=1.2.3.4:3863
                                             -asap=1.2.3.4:3863,[2000::1:2:3],9.8.7.6

-asapannounce=auto|address:port: Sets the multicast address and UDP port to send
                                 the ASAP Announces to. Use "auto" for default.
                                 Examples: -asapannounce=auto
                                           -asapannounce=239.0.0.1:3863

-maxbadpereports=reports: Sets the maximum number of ASAP Endpoint Unreachable reports before
                          removing a PE.

-endpointkeepalivetransmissioninterval=milliseconds: Sets the ASAP Endpoint Keep Alive interval.

-endpointkeepalivetimeoutinterval=milliseconds: Sets the ASAP Endpoint Keep Alive timeout.

-serverannouncecycle=milliseconds: Sets the ASAP Announce interval.

-autoclosetimeout=seconds: Sets the SCTP autoclose timeout for idle ASAP associations.

-minaddressscope: Sets the minimum address scope acceptable for registered PEs:
                  * loopback: Loopback address (only valid on the same node!)
                  * site-local: Site-local addresses (e.g. 192.168.1.1, etc.)
                  * global: Global addresses

-quiet: Do not print startup and shutdown messages.


## ENRP Parameters

-enrp=auto|address:port{,address}: Sets the ENRP endpoint address(es). Use "auto"
                                   to automatically set it (default).
                                   Examples: -enrp=auto
                                             -enrp=1.2.3.4:9901
                                             -enrp=1.2.3.4:9901,[2000::1:2:3],9.8.7.6

-enrpannounce=auto|address:port: Sets the multicast address and UDP port to send
                                 the ENRP Announces to. Use "auto" for default.
                                 Examples: -enrpannounce=auto
                                           -enrpannounce=239.0.0.1:9901

-peerheartbeatcycle=milliseconds: Sets the ENRP peer heartbeat interval.

-peermaxtimelastheard=milliseconds: Sets the ENRP peer max time last heard.

-peermaxtimenoresponse=milliseconds: Sets the ENRP maximum time without response.

-takeoverexpiryinterval=milliseconds: Sets the ENRP takeover timeout.

-mentorhuntinterval=milliseconds: Sets the mentor PR hunt interval.


# 📚 Component Status Protocol

The Component Status Protocol is a simple UDP-based protocol for RSerPool components to send their status to a central monitoring component. A console-based receiver is ./cspmonitor; it receives the status updates by
default on UDP port 2960.

In order to send status information, the registrar as well as all servers and clients described in section B provide two parameters:

-cspserver=Address:Port: Sets the CSP monitor server's address and port.

-cspinterval=Interval: Sets the interval for the CSP status updates in
                       milliseconds.

Note: Both parameters MUST be provided in order to send status updates!


# 📦 Binary Package Installation

Please use the issue tracker at [https://github.com/dreibh/rsplib/issues](https://github.com/dreibh/rsplib/issues) to report bugs and issues!

## Ubuntu Linux

For ready-to-install Ubuntu Linux packages of RSPLIB, see [Launchpad PPA for Thomas Dreibholz](https://launchpad.net/~dreibh/+archive/ubuntu/ppa/+packages?field.name_filter=rsplib&field.status_filter=published&field.series_filter=)!

<pre>
sudo apt-add-repository -sy ppa:dreibh/ppa
sudo apt-get update
sudo apt-get install rsplib
</pre>

## Fedora Linux

For ready-to-install Fedora Linux packages of RSPLIB, see [COPR PPA for Thomas Dreibholz](https://copr.fedorainfracloud.org/coprs/dreibh/ppa/package/rsplib/)!

<pre>
sudo dnf copr enable -y dreibh/ppa
sudo dnf install rsplib
</pre>

## FreeBSD

For ready-to-install FreeBSD packages of RSPLIB, it is included in the ports collection, see [FreeBSD ports tree index of net/rsplib/](https://cgit.freebsd.org/ports/tree/net/rsplib/)!

<pre>
pkg install rsplib
</pre>

Alternatively, to compile it from the ports sources:

<pre>
cd /usr/ports/net/rsplib
make
make install
</pre>


# 💾 Build from Sources

RSPLIB is released under the [GNU General Public Licence&nbsp;(GPL)](https://www.gnu.org/licenses/gpl-3.0.en.html#license-text).

Please use the issue tracker at [https://github.com/dreibh/rsplib/issues](https://github.com/dreibh/rsplib/issues) to report bugs and issues!

## Development Version

The Git repository of the RSPLIB sources can be found at [https://github.com/dreibh/rsplib](https://github.com/dreibh/rsplib):

<pre>
git clone https://github.com/dreibh/rsplib
cd rsplib
cmake .
make
</pre>

Contributions:

* Issue tracker: [https://github.com/dreibh/rsplib/issues](https://github.com/dreibh/rsplib/issues).
  Please submit bug reports, issues, questions, etc. in the issue tracker!

* Pull Requests for RSPLIB: [https://github.com/dreibh/rsplib/pulls](https://github.com/dreibh/rsplib/pulls).
  Your contributions to RSPLIB are always welcome!

* CI build tests of RSPLIB: [https://github.com/dreibh/rsplib/actions](https://github.com/dreibh/rsplib/actions).

* Coverity Scan analysis of RSPLIB: [https://scan.coverity.com/projects/dreibh-rsplib](https://scan.coverity.com/projects/dreibh-rsplib).

## Release Versions

See [https://www.nntb.no/~dreibh/rsplib/#current-stable-release](https://www.nntb.no/~dreibh/rsplib/#current-stable-release) for release packages!


# 🖋️ Citing RSPLIB in Publications

[Dreibholz, Thomas](https://www.nntb.no/~dreibh/): «[Reliable Server Pooling – Evaluation, Optimization and Extension of a Novel IETF Architecture](https://duepublico2.uni-due.de/servlets/MCRFileNodeServlet/duepublico_derivate_00016326/Dre2006_final.pdf)» ([PDF](https://duepublico2.uni-due.de/servlets/MCRFileNodeServlet/duepublico_derivate_00016326/Dre2006_final.pdf), 9080&nbsp;KiB, 267&nbsp;pages, 🇬🇧), University of Duisburg-Essen, Faculty of Economics, Institute for Computer Science and Business Information Systems, URN&nbsp;[urn:nbn:de:hbz:465-20070308-164527-0](https://nbn-resolving.org/urn:nbn:de:hbz:465-20070308-164527-0), March&nbsp;7, 2007.



A. FIRST STEPS

How to set up a simple test of the rsplib-3.x.y release:

1. If, AND ONLY IF, you would like to use the sctplib/socketapi *userland* SCTP
   implementation, do the following two steps first:

 1.a. Get latest sctplib-****.tar.gz archive and install it by
      - Unpack the Tar/GZip archive
      - ./configure --enable-shared --enable-static
      - make
      - make install
         (as root!)

 1.b. Get latest socketapi-****.tar.gz and install it by
      - Unpack the Tar/GZip archive
      - ./configure --enable-shared --enable-static
      - make
      - make install
         (as root!)

2. Get latest rsplib-****.tar.gz and install it by
      - Unpack the Tar/GZip archive
      - If, AND ONLY IF, you would like to use sctplib/socketapi SCTP
        userland implementation (if unsure, you do *NOT* want this!):
        cmake . -DUSE_KERNEL_SCTP=0
        OTHERWISE (i.e. for kernel SCTP):
        cmake .
      - make
      - make install
         (Only necessary if you want an installation; as root!)

      Optional "cmake" Parameters:
      -DMAX_LOGLEVEL=n Allows for reduction of the maximum logging verbosity to n.
                       Setting a lower value here makes the programs smaller, at
                       cost of reduces logging capabilities (DEFAULT: 9).

      -DENABLE_QT=1: Enables Qt 5.x usage; this is necessary for the Fractal Generator
                     client! Without Qt, the client's compilation will be skipped.
                    (DEFAULT)
      -DENABLE_QT=0: Disables Qt usage; the Fractal Generator client will *not*
                     be available.

      -DENABLE_REGISTRAR_STATISTICS=1: Adds registrar option to write statistics file.
                                       (DEFAULT)
      -DENABLE_REGISTRAR_STATISTICS=0: Do not compile in the statistics option. In this
                                       case, the dependency on libbz2 is removed.

      -DENABLE_HSMGTVERIFY=1: Enable Handlespace Management verification. This is
                              useful for debugging only (makes the PR slow!)
      -DENABLE_HSMGTVERIFY=0: Turns off Handlespace Management verification. (DEFAULT)

      -DENABLE_CSP=1: Enable the Component Status Protocol support (strongly recommended!)
                      (DEFAULT)
      -DENABLE_CSP=0: Turns the Component Status Protocol support off.

      -DBUILD_TEST_PROGRAMS=1: Enable building of test programs.
      -DBUILD_TEST_PROGRAMS=0: Disable building of test programs. (DEFAULT)

3. Ensure that multicast is working!
   - You need a configured network interface with:
        + at least a private address (192.168.x.y; 10.a.b.c; 172.16.n.m - 172.31.i.j)
        + having the multicast flag set (e.g. "ifconfig <dev> multicast")
     even if you want to make tests only via loopback! Multicast-announces do not
     work correctly otherwise!
     If you have a dummy device, configure it appropriately:
     root# ifconfig dummy0 10.255.255.1 netmask 255.255.255.0 broadcast 10.255.255.255 up multicast
   - Ensure that your firewall settings for the ethernet interface allow
     UDP packets to/from the registrar (ASAP Announce/ENRP Presence) as well
     as ASAP/ENRP traffic over SCTP.

4. Start at least one registrar
   ./rspregistrar
   See also Section D for registrar parameters.

5. Start at least one pool element
   ./rspserver -echo
   (You can start multiple pool elements; they may also run on different hosts, of course.
    If it complains about finding no registrar, check the multicast settings!)

6. Start a pool user
   ./rspterminal
   (If it complains about finding no registrar, check the multicast settings!)
   Now, you have to manually enter text lines on standard input.

7. If everything works, you can test RSerPool functionality by stopping the pool
   element and watching the failover.

8. You can monitor the status of each component using the Component Status Protocol
   monitor "cspmonitor". Simply start it by "./cspmonitor". It will listen for status
   messages sent via UDP on port 2960. The components (registrar, rspserver, etc.) accept
   the command line arguments "-cspserver=<Server>:<Port>" and
   "-cspinterval=<Microseconds>". For example, if you want a status update every 300ms
   and your CSP client is listening on port 2960 of host 1.2.3.4, use the arguments
   "-cspserver=1.2.3.4:2960 -cspinterval=300".
   NOTE: You *must* specify address AND interval, otherwise no messages are sent.

9. You can use Wireshark (https://www.wireshark.org) to observe the RSerPool and demo
   protocol traffic. Coloring rules and filters can be found in the directory
   "rsplib/wireshark" Simply copy "colorfilters", "dfilters" and optionally
   "preferences" to ~/.wireshark (or /root/.wireshark).
   Dissectors for the application protocols are already included in recent Wireshark
   distributions!



B. DEMO POOL ELEMENTS (SERVERS)

All demo servers can be started using the rspserver program. It takes a set of
common parameters as well as some service-specific arguments. These parameters
are explained in the following.

Note: For most of the provided services, the latest version of Wireshark
      (https://www.wireshark.org) already includes the packets dissectors!


B.1 Common Parameters

-loglevel=0-9: Sets the logging verbosity from 0 (none) to 9 (very verbose).

-logcolor=on|off: Turns ANSI colorization of the logging output on or off.

-logfile=filename: Write logging output to a file (default is stdout).

-poolhandle=PH: Sets the PH to a non-default value; otherwise, the default
                setting will be the service-specific default (see below).

-registrar=Address:Port: Adds a static PR entry into the Registrar Table.
                         It is possible to add multiple entries.

-asapannounce=Address:Port: Sets the multicast address and port the ASAP instance
                            listens for ASAP Server Announces on.

-rereginterval=milliseconds: Sets the PE's re-registration interval (in ms).

-runtime=seconds: After the configured amount of seconds, the service is
                  shut down.

-quiet: Do not print startup and shutdown messages.

-policy=Policy...: Sets the pool policy and its parameters:
 Valid Settings:
 - Random
 - WeightedRandom:Weight
 - RoundRobin
 - WeightedRoundRobin:Weight
 - LeastUsed
 - LeastUsedDegradation:Degradation (0 to 1)


B.2 Echo Service

-echo: Selects Echo service. The default PH will be "EchoPool".


B.3 Discard Service

-discard: Selects Discard service. The default PH will be "DiscardPool".


B.4 Daytime Service

-daytime: Selects Daytime service. The default PH will be "DaytimePool".


B.5 Character Generator Service

-chargen: Selects Character Generator service. The default PH will be "ChargenPool".


B.6 Ping Pong Service

-pingpong: Selects Ping Pong service. The default PH will be "PingPongPool".

-pppfailureafter=number: After the set number of packets, the server will terminate
                         the connection in order to test failovers.

-pppmaxthreads=threads: Sets the maximum number of simultaneous sessions.


B.7 Fractal Generator Service

-fractal: Selects the Fractal Generator service. The default PH will be "FractalGeneratorPool".

-fgpcookiemaxtime: Send cookie after given number of milliseconds.

-fgpcookiemaxpackets: Send cookie after given number of Data messages.

-fgptransmittimeout: Set transmit timeout in milliseconds (timeout for rsp_sendmsg()).

-fgptestmode: Generate simple test pattern instead of calculating a fractal
              graphics (useful to conserve CPU power).

-fgpfailureafter=number: After the set number of data packets, the server will
                         terminate the connection in order to test failovers.

-fgpmaxthreads=threads: Sets the maximum number of simultaneous sessions.



C. DEMO POOL USERS (CLIENTS)

C.1 Common Parameters

-loglevel=0-9: Sets the logging verbosity from 0 (none) to 9 (very verbose).

-logcolor=on|off: Turns ANSI colorization of the logging output on or off.

-logfile=filename: Write logging output to a file (default is stdout).

-poolhandle=PH: Sets the PH.


C.2 Terminal (e.g. for Echo, Discard, Daytime and Chargen services)

Start with ./rspterminal; the default PH is EchoPool.


C.3 Fractal Client (for the Fractal Generator service)

./fractalpooluser

-configdir=Directory: Sets a directory to look for FGP config files. From all FGP files
                      (pattern: *.fgp) in this directory, random files are selected for the
                      calculation of requests.

-threads=Count:       Sets the number of parallel sessions for the calculation
                      of an image.

-caption=Title:       Sets the window title.


C.4 Ping Pong Client (for the Fractal Generator service)

./pingpong

-interval=milliseconds: Sets the Ping interval.



D. REGISTRAR

Start the registrar with:

./rspregistrar



D.1 Common Parameters

-loglevel=0-9: Sets the logging verbosity from 0 (none) to 9 (very verbose).

-logcolor=on|off: Turns ANSI colorization of the logging output on or off.

-logfile=filename: Write logging output to a file (default is stdout).

-peer=Address:Port: Adds a static PR entry into the Peer List.
                    It is possible to add multiple entries.


D.2 ASAP Parameters

-asap=auto|address:port{,address}: Sets the ASAP endpoint address(es). Use "auto"
                                   to automatically set it (default).
                                   Examples: -asap=auto
                                             -asap=1.2.3.4:3863
                                             -asap=1.2.3.4:3863,[2000::1:2:3],9.8.7.6

-asapannounce=auto|address:port: Sets the multicast address and UDP port to send
                                 the ASAP Announces to. Use "auto" for default.
                                 Examples: -asapannounce=auto
                                           -asapannounce=239.0.0.1:3863

-maxbadpereports=reports: Sets the maximum number of ASAP Endpoint Unreachable reports before
                          removing a PE.

-endpointkeepalivetransmissioninterval=milliseconds: Sets the ASAP Endpoint Keep Alive interval.

-endpointkeepalivetimeoutinterval=milliseconds: Sets the ASAP Endpoint Keep Alive timeout.

-serverannouncecycle=milliseconds: Sets the ASAP Announce interval.

-autoclosetimeout=seconds: Sets the SCTP autoclose timeout for idle ASAP associations.

-minaddressscope: Sets the minimum address scope acceptable for registered PEs:
                  * loopback: Loopback address (only valid on the same node!)
                  * site-local: Site-local addresses (e.g. 192.168.1.1, etc.)
                  * global: Global addresses

-quiet: Do not print startup and shutdown messages.


D.3 ENRP Parameters

-enrp=auto|address:port{,address}: Sets the ENRP endpoint address(es). Use "auto"
                                   to automatically set it (default).
                                   Examples: -enrp=auto
                                             -enrp=1.2.3.4:9901
                                             -enrp=1.2.3.4:9901,[2000::1:2:3],9.8.7.6

-enrpannounce=auto|address:port: Sets the multicast address and UDP port to send
                                 the ENRP Announces to. Use "auto" for default.
                                 Examples: -enrpannounce=auto
                                           -enrpannounce=239.0.0.1:9901

-peerheartbeatcycle=milliseconds: Sets the ENRP peer heartbeat interval.

-peermaxtimelastheard=milliseconds: Sets the ENRP peer max time last heard.

-peermaxtimenoresponse=milliseconds: Sets the ENRP maximum time without response.

-takeoverexpiryinterval=milliseconds: Sets the ENRP takeover timeout.

-mentorhuntinterval=milliseconds: Sets the mentor PR hunt interval.


E. COMPONENT STATUS PROTOCOL

The Component Status Protocol is a simple UDP-based protocol for RSerPool
components to send their status to a central monitoring component. A
console-based receiver is ./cspmonitor; it receives the status updates by
default on UDP port 2960.

In order to send status information, the registrar as well as all servers and
clients described in section B provide two parameters:

-cspserver=Address:Port: Sets the CSP monitor server's address and port.

-cspinterval=Interval: Sets the interval for the CSP status updates in
                       milliseconds.

Note: Both parameters MUST be provided in order to send status updates!



==================================================================================
Please subscribe to our mailing list and report any problems you discover!
Help us to improve rsplib, the world's first Open Source RSerPool implementation!

Visit https://www.nntb.no/~dreibh/rserpool/ for more information
==================================================================================


04.10.2019 Thomas Dreibholz, thomas.dreibholz@gmail.com
