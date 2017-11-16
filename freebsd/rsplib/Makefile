# Created by: Thomas Dreibholz <dreibh@iem.uni-due.de>
# $FreeBSD: head/net/rsplib/Makefile 373063 2014-11-22 14:43:23Z antoine $

PORTNAME=	rsplib
PORTVERSION=	3.1.8
CATEGORIES=	net
MASTER_SITES=	https://www.uni-due.de/~be0001/rserpool/download/

MAINTAINER=	dreibh@iem.uni-due.de
COMMENT=	RSerPool Framework and Example Applications

LICENSE=	GPLv3

GNU_CONFIGURE=	yes
CONFIGURE_ARGS=	--disable-maintainer-mode \
		--enable-qt --with-qt-lib=${QT_LIBDIR} \
		--with-qt-include=${QT_INCDIR}
INSTALL_TARGET=	install-strip
USES=		gmake libtool shebangfix
SHEBANG_FILES=	rsplib/scriptingserviceexample
USE_LDCONFIG=	yes
USE_QT5=	buildtools_build core gui xml

.include <bsd.port.mk>
