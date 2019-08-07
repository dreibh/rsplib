# Created by: Thomas Dreibholz <dreibh@iem.uni-due.de>
# $FreeBSD: head/net/rsplib/Makefile 373063 2014-11-22 14:43:23Z antoine $

PORTNAME=	rsplib
PORTVERSION=	3.2.4
CATEGORIES=	net
MASTER_SITES=	https://www.uni-due.de/~be0001/rserpool/download/

MAINTAINER=	dreibh@iem.uni-due.de
COMMENT=	RSerPool Framework and Example Applications
EXTRACT_SUFX=   .tar.xz

LICENSE=	GPLv3+
LICENSE_FILE=   ${WRKSRC}/COPYING

GNU_CONFIGURE=	yes
USES=		cmake shebangfix
CMAKE_ARGS+=    -DCMAKE_INSTALL_MANDIR=${PREFIX}/man -DUSE_KERNEL_SCTP=1 -DENABLE_CSP=1 -DENABLE_QT=1
SHEBANG_FILES=	src/scriptingserviceexample
USE_LDCONFIG=	yes
USE_QT5=	buildtools_build core gui widgets xml

.include <bsd.port.mk>
