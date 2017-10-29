# Created by: Thomas Dreibholz <dreibh@iem.uni-due.de>
# $FreeBSD: net/rsplib/Makefile 327755 2013-09-20 22:10:23Z bapt $

PORTNAME=	rsplib
PORTVERSION=	3.1.7
CATEGORIES=	net
MASTER_SITES=	https://www.uni-due.de/~be0001/rserpool/

MAINTAINER=	dreibh@iem.uni-due.de
COMMENT=	RSerPool Framework and Example Applications

LICENSE=	GPLv3

USE_QT5=	gui xml
USE_GMAKE=	yes
GNU_CONFIGURE=	yes
CONFIGURE_ARGS=	--disable-maintainer-mode \
		--enable-qt --with-qt-lib=${QT_LIBDIR} \
		--with-qt-include=${QT_INCDIR}
USE_LDCONFIG=	yes

MAN1=		calcappclient.1 \
		cspmonitor.1 \
		fractalpooluser.1 \
		hsdump.1 \
		pingpongclient.1 \
		registrar.1 \
		rspregistrar.1 \
		rspserver.1 \
		rspterminal.1 \
		scriptingclient.1 \
		scriptingcontrol.1 \
		scriptingserviceexample.1 \
		server.1 \
		terminal.1


NO_STAGE=	yes
.include <bsd.port.mk>
