PORTNAME=	rsplib
DISTVERSION=	3.5.0~rc0
CATEGORIES=	net
MASTER_SITES=	https://www.nntb.no/~dreibh/rserpool/download/

MAINTAINER=	thomas.dreibholz@gmail.com
COMMENT=	RSerPool Framework and Example Applications
WWW=		https://www.nntb.no/~dreibh/rserpool/

LICENSE=	GPLv3+
LICENSE_FILE=	${WRKSRC}/COPYING

USES=		cmake qt:5 tar:xz
BUILD_DEPENDS=	gm:graphics/GraphicsMagick \
		pdf2svg:graphics/pdf2svg \
		gs:print/ghostscript10
USE_LDCONFIG=	yes
USE_QT=		buildtools:build core gui qmake widgets xml

PLIST_SUB=	DISTVERSION=${DISTVERSION}

CMAKE_ARGS=	-DCMAKE_INSTALL_DOCDIR=${DOCSDIR} \
		-DUSE_KERNEL_SCTP=1 -DENABLE_CSP=1 -DENABLE_QT=1

.include <bsd.port.mk>
