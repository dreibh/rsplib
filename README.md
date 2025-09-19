<h1 align="center">
 RSPLIB<br />
 <span style="font-size: 30%">The Reliable Server Pooling Implementation</span><br />
 <a href="https://www.nntb.no/~dreibh/rserpool/">
  <img alt="RSPLIB Project Logo" src="src/logo/rsplib.svg" width="25%" /><br />
  <span style="font-size: 30%;">https://www.nntb.no/~dreibh/rserpool</span>
 </a>
</h1>


# 💡 What is RSPLIB?

[Reliable Server Pooling&nbsp;(RSerPool)](https://www.nntb.no/~dreibh/rserpool/#Description) is the new IETF framework for server pool management and session failover handling. In particular, it can be used for realising highly available services and load distribution. RSPLIB is the reference implementation of RSerPool. It includes:

* The library librsplib, which is the RSerPool implementation itself;
* The library libcpprspserver, which is a C++ wrapper library to easily write server applications based on librsplib;
* A collection of server (pool element) and client (pool user) examples.


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

All demo PE services can be started using the ```rspserver``` program. That is:

<pre>
rspserver &lt;options&gt; ...
</pre>

It takes a set of common parameters as well as some service-specific arguments. These parameters are explained in the following.

Notes:

* For most of the provided services, the latest version of [Wireshark](https://www.wireshark.org) already includes the packets dissectors!
* See the [manpage of "rspserver"](https://github.com/dreibh/rsplib/blob/master/src/rspserver.1) for further options!

  <pre>
  man rspserver
  </pre>


## Common Parameters

```rspserver``` provides some common options for all services:

* ```-loglevel=0-9```: Sets the logging verbosity from 0 (none) to 9 (very verbose).
* ```-logcolor=on|off```: Turns ANSI colorization of the logging output on or off.
* ```-logfile=<filename>```: Writes logging output to a file (default is stdout).
* ```-poolhandle=<poolhandle>```: Sets the PH to a non-default value; otherwise, the default setting will be the service-specific default (see below).
* ```-cspserver=<address>:<port>```: See [Component Status Protocol](#component-status-protocol) below).
* ```-cspinterval=<milliseconds>```: See [Component Status Protocol](#component-status-protocol) below).
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

Note: The Echo Service will be started by default, unless a different service is specified!

Example:

<pre>
rspserver -echo -poolhandle=MyEchoPool
</pre>


## Discard Service

```-discard```: Selects Discard service. The default PH will be "DiscardPool".

Example:

<pre>
rspserver -discard -poolhandle=MyDiscardPool
</pre>


## Daytime Service

```-daytime```: Selects Daytime service. The default PH will be "DaytimePool".

Example:

<pre>
rspserver -daytime -poolhandle=MyDaytimePool
</pre>


## Character Generator Service

```-chargen```: Selects Character Generator service. The default PH will be "CharGenPool".

Example:

<pre>
rspserver -chargen -poolhandle=MyCharGenPool
</pre>


## Ping Pong Service

```-pingpong```: Selects Ping Pong service. The default PH will be "PingPongPool".

The Ping Pong service provides further options:

* ```-pppfailureafter=<number_of_messages>```: After the set number of messages, the server will terminate the connection in order to test failovers.
* ```-pppmaxthreads=<threads>```: Sets the maximum number of simultaneous sessions.

Example:

<pre>
rspserver -pingpong -poolhandle=MyPingPongPool -pppmaxthreads=4 -pppmaxthreads=8
</pre>


## Fractal Generator Service

```-fractal```: Selects the Fractal Generator service. The default PH will be "FractalGeneratorPool".

The Fractal Generator service provides further options:

* ```-fgpcookiemaxtime=<milliseconds>```: Send cookie after given number of milliseconds.
* ```-fgpcookiemaxpackets=<numner_of_messages>```: Send cookie after given number of Data messages.
* ```-fgptransmittimeout=<milliseconds>```: Set transmit timeout in milliseconds (timeout for rsp_sendmsg()).
* ```-fgptestmode```: Generate simple test pattern instead of calculating a fractal graphics (useful to conserve CPU power).
* ```-fgpfailureafter=<number_of_messages>```: After the set number of Data messages, the server will terminate the connection in order to test failovers.
* ```-fgpmaxthreads=<threads>```: Sets the maximum number of simultaneous sessions.

Example:

<pre>
rspserver -fractal -fgpmaxthreads=4
</pre>


# 📚 Pool Users (Clients)

## Common Parameters

The pool users provides some common options for all programs:

* ```-loglevel=0-9```: Sets the logging verbosity from 0 (none) to 9 (very verbose).
* ```-logcolor=on|off```: Turns ANSI colorization of the logging output on or off.
* ```-logfile=<filename>```: Writes logging output to a file (default is stdout).
* ```-poolhandle=<poolhandle>```: Sets the PH to a non-default value; otherwise, the default setting will be the service-specific default (see below).
* ```-cspserver=<address>:<port>```: See [Component Status Protocol](#component-status-protocol) below).
* ```-cspinterval=<milliseconds>```: See [Component Status Protocol](#component-status-protocol) below).
* ```-registrar=<address>:<port>```: Adds a static PR entry into the Registrar Table. It is possible to add multiple entries.


## Terminal Client

The PU for the
[Echo Service](#echo-service),
[Discard Service](#discard-service),
[Daytime Service](#daytime-service), or
[Character Generator Service](#character-generator-service)
can be started by:

```
rspterminal &lt;options&gt; ...
```

Input from standard input is sent to the PE, and the response is printed to standard output.

Example:

<pre>
rspterminal -poolhandle=MyDaytimePool
</pre>

Notes:

* The default PH is EchoPool. Use ```-poolhandle=<poolhandle>``` to set a different PH, e.g. "DaytimePool".
* See the [manpage of "rspterminal"](https://github.com/dreibh/rsplib/blob/master/src/rspterminal.1) for further options!
  <pre>
  man rspterminal
  </pre>


## Fractal Generator Client

The PU for the [Fractal Generator Service](#fractal-generator-service) can be started by:

```
fractalpooluser &lt;options&gt; ...
```

The Fractal Generator PU provides further options:

* ```-configdir=<directory>```: Sets a directory to look for FGP config files. From all FGP files (pattern: <tt>*.fgp</tt>) in this directory, random files are selected for the calculation of requests. The <tt>.fgp</tt> files can be created, read and modified by [FractGen][https://www.nntb.no/~dreibh/fractalgenerator/).
* ```-threads=<maximum_number_of_threads> ```: Sets the number of parallel sessions for the calculation of an image.
* ```-caption=<title>```: Sets the window title.

Example (assuming the <tt>.fgp</tt> input files are installed under <tt>/usr/share/fgpconfig</tt>):

<pre>
fractalpooluser -configdir=/usr/share/fgpconfig -caption="Fractal PU Demo!"
</pre>

Note: See the [manpage of "fractalpooluser"](https://github.com/dreibh/rsplib/blob/master/src/fractalpooluser.1) for further options!

<pre>
man fractalpooluser
</pre>


## Ping Pong Client

The PU for the [Ping Pong Service](#ping-pong-service) can be started by:

```
pingpongclient
```

The Ping Pong PU provides further options:

* ```-interval=<milliseconds>```: Sets the Ping interval in milliseconds.

Example:

<pre>
pingpongclient -poolhandle=MyPingPongPool -interval=333
</pre>

Note: See the [manpage of "pingpongclient"](https://github.com/dreibh/rsplib/blob/master/src/pingpongclient.1) for further options!

<pre>
man pingpongclient
</pre>


# 📚 Registrar

Start the registrar with:

<pre>
rspregistrar &lt;options&gt; ...
</pre>


## Basic Parameters

* ```-loglevel=0-9```: Sets the logging verbosity from 0 (none) to 9 (very verbose).
* ```-logcolor=on|off```: Turns ANSI colorization of the logging output on or off.
* ```-logfile=<filename>```: Writes logging output to a file (default is stdout).
* ```-cspserver=<address>:<port>```: See [Component Status Protocol](#component-status-protocol) below).
* ```-cspinterval=<milliseconds>```: See [Component Status Protocol](#component-status-protocol) below).


## ASAP Parameters

* ```-asap=auto|<address>:<port>[<,address>]```: Sets the ASAP endpoint address(es). Use "auto" to automatically set it (default). Examples:
  - ```-asap=auto```
  - ```-asap=1.2.3.4:3863```
  - ```-asap=1.2.3.4:3863,[2000::1:2:3],9.8.7.6```

* ```-asapannounce=auto|<address>:<port>```: Sets the multicast address and UDP port to send the ASAP Announces to. Use "auto" for default. Examples:
  - ```-asapannounce=auto```
  - ```-asapannounce=239.0.0.1:3863```
* ```-maxbadpereports=<number_of_reports>```: Sets the maximum number of ASAP Endpoint Unreachable reports before removing a PE.
* ```-endpointkeepalivetransmissioninterval=<milliseconds>```: Sets the ASAP Endpoint Keep Alive interval.
* ```-endpointkeepalivetimeoutinterval=<milliseconds>```: Sets the ASAP Endpoint Keep Alive timeout.
* ```-serverannouncecycle=<milliseconds>```: Sets the ASAP Announce interval.
* ```-autoclosetimeout=<seconds>```: Sets the SCTP autoclose timeout for idle ASAP associations.
* ```-minaddressscope=<scope>```: Sets the minimum address scope acceptable for registered PEs:
  - ```loopback```: Loopback address (only valid on the same node!)
  - ```site-local```: Site-local addresses (e.g. 192.168.1.1, etc.)
  - ```global```: Global addresses
* ```-quiet```: Do not print startup and shutdown messages.


## ENRP Parameters

* ```-enrp=auto|<address>:<port>[<,address>]```: Sets the ENRP endpoint address(es). Use "auto" to automatically set it (default). Examples:
  - ```-enrp=auto```
  - ```-enrp=1.2.3.4:9901```
  - ```-enrp=1.2.3.4:9901,[2000::1:2:3],9.8.7.6```
* ```-enrpannounce=auto|<address>:<port>```: Sets the multicast address and UDP port to send the ENRP Announces to. Use "auto" for default. Examples:
  - ```-enrpannounce=auto```
  - ```-enrpannounce=239.0.0.1:9901```
* ```-peer=<address>:<port>```: Adds a static PR entry into the Peer List. It is possible to add multiple entries.
* ```-peerheartbeatcycle=<milliseconds>```: Sets the ENRP peer heartbeat interval.
* ```-peermaxtimelastheard=<milliseconds>```: Sets the ENRP peer max time last heard.
* ```-peermaxtimenoresponse=<milliseconds>```: Sets the ENRP maximum time without response.
* ```-takeoverexpiryinterval=<milliseconds>```: Sets the ENRP takeover timeout.
* ```-mentorhuntinterval=<milliseconds>```: Sets the mentor PR hunt interval.


## Further Parameters

Note: See the [manpage of "rspregistrar"](https://github.com/dreibh/rsplib/blob/master/src/rspregistrar.1) for further options!

<pre>
man rspregistrar
</pre>


# 📚 Component Status Protocol

The Component Status Protocol is a simple UDP-based protocol for RSerPool components to send their status to a central monitoring component. A console-based receiver is ./cspmonitor; it receives the status updates by
default on UDP port 2960.

In order to send status information, the registrar as well as all servers and clients described in section B provide two parameters:

* ```-cspserver=<address>:<port>```: Sets the CSP monitor server's address and port.
* ```-cspinterval=<milliseconds>```: Sets the interval for the CSP status updates in milliseconds.

Note: Both parameters **must** be provided in order to send status updates!


# 🖋️ Citing RSPLIB in Publications

[Dreibholz, Thomas](https://www.nntb.no/~dreibh/): «[Reliable Server Pooling – Evaluation, Optimization and Extension of a Novel IETF Architecture](https://duepublico2.uni-due.de/servlets/MCRFileNodeServlet/duepublico_derivate_00016326/Dre2006_final.pdf)» ([PDF](https://duepublico2.uni-due.de/servlets/MCRFileNodeServlet/duepublico_derivate_00016326/Dre2006_final.pdf), 9080&nbsp;KiB, 267&nbsp;pages, 🇬🇧), University of Duisburg-Essen, Faculty of Economics, Institute for Computer Science and Business Information Systems, URN&nbsp;[urn:nbn:de:hbz:465-20070308-164527-0](https://nbn-resolving.org/urn:nbn:de:hbz:465-20070308-164527-0), March&nbsp;7, 2007.
