.if "${.OBJDIR}" == "${.CURDIR}"

obj:
	mkdir -p ${.CURDIR}/obj

.else

SUDO?=	doas
PROG=	ali.frad.ir
SRCS+=	fastcgi.c cgi.c www.c
SRCS+=	www_os.c
SRCS+=	www_games.c
SRCS+=	www_utils.c
MAN=

DEBUD=	-g

CFLAGS+= -Wall -Werror
CFLAGS+= -Wstrict-prototypes -Wmissing-prototypes
CFLAGS+= -Wmissing-declarations
CFLAGS+= -Wshadow -Wpointer-arith -Wcast-qual
CFLAGS+= -Wsign-compare
CFLAGS+= -I. -pthread
LDFLAGS+= -pthread
LDADD=	-levent -lpthread -lz
DPADD=	${LIBEVENT} ${LIBZ}

### lang

SRCS+=		lang.c
CLEANFILES+=	lang_parser lang.c lang.h

lang.c: lang_parser lang.csv
	cat ${.CURDIR}/lang.csv | ./lang_parser c > $@

lang.h: lang_parser lang.csv
	cat ${.CURDIR}/lang.csv | ./lang_parser h > $@

lang_parser: lang_parser.c
	${CC} ${.CURDIR}/lang_parser.c -lc -o $@

### res

RES_DIR=	${.CURDIR}/res
RES_FILES!=	find ${RES_DIR} -type f | sort -V
SRCS+=		res.c
CLEANFILES+=	res_parser res.c res.h

res.c: res_parser ${RES_DIR}
	ls ${RES_FILES} | ./res_parser c ${RES_DIR} > $@

res.h: res_parser ${RES_DIR}
	ls ${RES_FILES} | ./res_parser h ${RES_DIR} > $@

res_parser: res_parser.c
	${CC} ${.CURDIR}/res_parser.c -lc -o $@

###

.depend ${SRCS:N*.h:N*.sh:R:S/$/.o/}: lang.h res.h

test: ${PROG}
	${SUDO} ${PROG} -d

clean:
	rm -rf ${.CURDIR}/obj/*

.PHONY: obj test clean

.include <bsd.prog.mk>

.endif
