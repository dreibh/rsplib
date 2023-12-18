PORTNAME=	rsplib
PORTVERSION=	3.4.5
CATEGORIES=	net
MASTER_SITES=	https://www.nntb.no/~dreibh/rserpool/download/

MAINTAINER=	thomas.dreibholz@gmail.com
COMMENT=	RSerPool Framework and Example Applications
WWW=		https://www.nntb.no/~dreibh/rserpool/

LICENSE=	GPLv3+
LICENSE_FILE=	${WRKSRC}/COPYING

USES=		cmake compiler:c++11-lang qt:5 shebangfix tar:xz
USE_LDCONFIG=	yes
USE_QT=		buildtools:build core gui qmake widgets xml

CMAKE_ARGS+=	-DCMAKE_INSTALL_MANDIR=${PREFIX}/man \
		-DDOCSDIR="${DOCSDIR}" \
		-DUSE_KERNEL_SCTP=1 -DENABLE_CSP=1 -DENABLE_QT=1
SHEBANG_FILES=	src/scriptingserviceexample

PLIST_SUB=	VERSION=${PORTVERSION}

.include <bsd.port.mk>
