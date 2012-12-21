# New ports collection makefile for:	rsplib
# Date created:		2011-11-18
# Whom:			Thomas Dreibholz <dreibh@iem.uni-due.de>
#
# $FreeBSD: ports/net/rsplib/Makefile,v 1.2 2011/03/03 17:09:45 sylvio Exp $
#

PORTNAME=	rsplib
PORTVERSION=	3.0.1
CATEGORIES=	net
MASTER_SITES=	http://www.tdr.wiwi.uni-due.de/fileadmin/fileupload/I-TDR/ReliableServer/Downloads/ http://www.exp-math.uni-essen.de/~dreibh/rserpool/

MAINTAINER=	dreibh@iem.uni-due.de
COMMENT=	RSerPool Framework and Example Applications

USE_QT_VER=	4
QT_COMPONENTS=	gui xml moc_build
USE_GMAKE=	yes
GNU_CONFIGURE=	yes
CONFIGURE_ARGS=	--disable-maintainer-mode \
		--enable-qt --with-qt-lib=${QT_LIBDIR} \
		--with-qt-include=${QT_INCDIR}
USE_LDCONFIG=	yes

LICENSE=	GPLv3
LICENSE_FILE=	${WRKSRC}/COPYING

MAN1=		calcappclient.1 \
		cspmonitor.1 \
		fractalpooluser.1 \
		hsdump.1 \
		pingpongclient.1 \
		registrar.1 \
		scriptingclient.1 \
		scriptingcontrol.1 \
		scriptingserviceexample.1 \
		server.1 \
		terminal.1

MAKE_JOBS_SAFE=	yes

.include <bsd.port.pre.mk>

.if ${OSVERSION} < 700028
IGNORE=		requires sctp(4) support
.endif

.include <bsd.port.post.mk>
