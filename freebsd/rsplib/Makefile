# New ports collection makefile for: rsplib
# Date created:                      27 July 2010
# Whom:                              Thomas Dreibholz <dreibh@iem.uni-due.de>
#
# $FreeBSD$
#

PORTNAME=	rsplib
PORTVERSION=	2.7.5
CATEGORIES=	net
MASTER_SITES=	http://www.tdr.wiwi.uni-due.de/fileadmin/fileupload/I-TDR/ReliableServer/Downloads/
DISTNAME=	${PORTNAME}-${PORTVERSION}

MAINTAINER=     dreibh@iem.uni-due.de
COMMENT=        RSerPool Framework and Example Applications

# RUN_DEPENDS=
LIB_DEPENDS=    QtGui.4:${PORTSDIR}/x11-toolkits/qt4-gui

GNU_CONFIGURE=  yes
CONFIGURE_ARGS=	"--enable-qt=/usr/local"
USE_GMAKE=      yes

LICENSE=        GPLv3
LICENSE_FILE=   ${WRKSRC}/COPYING

pre-fetch:

post-patch:

pre-install:

.include <bsd.port.mk>
