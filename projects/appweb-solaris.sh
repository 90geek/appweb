#
#   appweb-solaris.sh -- Build It Shell Script to build Embedthis Appweb
#

ARCH="x86"
ARCH="$(shell uname -m | sed 's/i.86/x86/;s/x86_64/x64/')"
OS="solaris"
PROFILE="debug"
CONFIG="${OS}-${ARCH}-${PROFILE}"
CC="/usr/bin/gcc"
LD="/usr/bin/ld"
CFLAGS="-Wall -fPIC -g -Wshorten-64-to-32 -mtune=generic"
DFLAGS="-D_REENTRANT -DPIC -DBIT_DEBUG"
IFLAGS="-I${CONFIG}/inc"
LDFLAGS="-g"
LIBPATHS="-L${CONFIG}/bin"
LIBS="-llxnet -lrt -lsocket -lpthread -lm -ldl"

[ ! -x ${CONFIG}/inc ] && mkdir -p ${CONFIG}/inc ${CONFIG}/obj ${CONFIG}/lib ${CONFIG}/bin

[ ! -f ${CONFIG}/inc/bit.h ] && cp projects/appweb-${OS}-bit.h ${CONFIG}/inc/bit.h
if ! diff ${CONFIG}/inc/bit.h projects/appweb-${OS}-bit.h >/dev/null ; then
	cp projects/appweb-${OS}-bit.h ${CONFIG}/inc/bit.h
fi

rm -rf ${CONFIG}/inc/mpr.h
cp -r src/deps/mpr/mpr.h ${CONFIG}/inc/mpr.h

${CC} -c -o ${CONFIG}/obj/mprLib.o -Wall -fPIC ${LDFLAGS} -Wshorten-64-to-32 -mtune=generic ${DFLAGS} -I${CONFIG}/inc src/deps/mpr/mprLib.c

${CC} -shared -o ${CONFIG}/bin/libmpr.so ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/mprLib.o ${LIBS}

${CC} -c -o ${CONFIG}/obj/mprSsl.o -Wall -fPIC ${LDFLAGS} -Wshorten-64-to-32 -mtune=generic ${DFLAGS} -I${CONFIG}/inc src/deps/mpr/mprSsl.c

${CC} -shared -o ${CONFIG}/bin/libmprssl.so ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/mprSsl.o ${LIBS} -lmpr

${CC} -c -o ${CONFIG}/obj/manager.o -Wall -fPIC ${LDFLAGS} -Wshorten-64-to-32 -mtune=generic ${DFLAGS} -I${CONFIG}/inc src/deps/mpr/manager.c

${CC} -o ${CONFIG}/bin/appman ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/manager.o ${LIBS} -lmpr ${LDFLAGS}

rm -rf ${CONFIG}/inc/pcre.h
cp -r src/deps/pcre/pcre.h ${CONFIG}/inc/pcre.h

${CC} -c -o ${CONFIG}/obj/pcre.o -Wall -fPIC ${LDFLAGS} -Wshorten-64-to-32 -mtune=generic ${DFLAGS} -I${CONFIG}/inc src/deps/pcre/pcre.c

${CC} -shared -o ${CONFIG}/bin/libpcre.so ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/pcre.o ${LIBS}

rm -rf ${CONFIG}/inc/http.h
cp -r src/deps/http/http.h ${CONFIG}/inc/http.h

${CC} -c -o ${CONFIG}/obj/httpLib.o -Wall -fPIC ${LDFLAGS} -Wshorten-64-to-32 -mtune=generic ${DFLAGS} -I${CONFIG}/inc src/deps/http/httpLib.c

${CC} -shared -o ${CONFIG}/bin/libhttp.so ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/httpLib.o ${LIBS} -lpam -lmpr -lpcre -lmprssl

rm -rf ${CONFIG}/inc/sqlite3.h
cp -r src/deps/sqlite/sqlite3.h ${CONFIG}/inc/sqlite3.h

${CC} -c -o ${CONFIG}/obj/sqlite3.o -fPIC ${LDFLAGS} -mtune=generic -w ${DFLAGS} -I${CONFIG}/inc src/deps/sqlite/sqlite3.c

${CC} -shared -o ${CONFIG}/bin/libsqlite3.so ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/sqlite3.o ${LIBS}

rm -rf ${CONFIG}/inc/appweb.h
cp -r src/appweb.h ${CONFIG}/inc/appweb.h

rm -rf ${CONFIG}/inc/customize.h
cp -r src/customize.h ${CONFIG}/inc/customize.h

${CC} -c -o ${CONFIG}/obj/config.o -Wall -fPIC ${LDFLAGS} -Wshorten-64-to-32 -mtune=generic ${DFLAGS} -I${CONFIG}/inc src/config.c

${CC} -c -o ${CONFIG}/obj/convenience.o -Wall -fPIC ${LDFLAGS} -Wshorten-64-to-32 -mtune=generic ${DFLAGS} -I${CONFIG}/inc src/convenience.c

${CC} -c -o ${CONFIG}/obj/dirHandler.o -Wall -fPIC ${LDFLAGS} -Wshorten-64-to-32 -mtune=generic ${DFLAGS} -I${CONFIG}/inc src/dirHandler.c

${CC} -c -o ${CONFIG}/obj/fileHandler.o -Wall -fPIC ${LDFLAGS} -Wshorten-64-to-32 -mtune=generic ${DFLAGS} -I${CONFIG}/inc src/fileHandler.c

${CC} -c -o ${CONFIG}/obj/log.o -Wall -fPIC ${LDFLAGS} -Wshorten-64-to-32 -mtune=generic ${DFLAGS} -I${CONFIG}/inc src/log.c

${CC} -c -o ${CONFIG}/obj/server.o -Wall -fPIC ${LDFLAGS} -Wshorten-64-to-32 -mtune=generic ${DFLAGS} -I${CONFIG}/inc src/server.c

${CC} -shared -o ${CONFIG}/bin/libappweb.so ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/config.o ${CONFIG}/obj/convenience.o ${CONFIG}/obj/dirHandler.o ${CONFIG}/obj/fileHandler.o ${CONFIG}/obj/log.o ${CONFIG}/obj/server.o ${LIBS} -lhttp -lpam -lmpr -lpcre -lmprssl

rm -rf ${CONFIG}/inc/edi.h
cp -r src/esp/edi.h ${CONFIG}/inc/edi.h

rm -rf ${CONFIG}/inc/esp-app.h
cp -r src/esp/esp-app.h ${CONFIG}/inc/esp-app.h

rm -rf ${CONFIG}/inc/esp.h
cp -r src/esp/esp.h ${CONFIG}/inc/esp.h

rm -rf ${CONFIG}/inc/mdb.h
cp -r src/esp/mdb.h ${CONFIG}/inc/mdb.h

${CC} -c -o ${CONFIG}/obj/edi.o -Wall -fPIC ${LDFLAGS} -Wshorten-64-to-32 -mtune=generic ${DFLAGS} -I${CONFIG}/inc src/esp/edi.c

${CC} -c -o ${CONFIG}/obj/espAbbrev.o -Wall -fPIC ${LDFLAGS} -Wshorten-64-to-32 -mtune=generic ${DFLAGS} -I${CONFIG}/inc src/esp/espAbbrev.c

${CC} -c -o ${CONFIG}/obj/espFramework.o -Wall -fPIC ${LDFLAGS} -Wshorten-64-to-32 -mtune=generic ${DFLAGS} -I${CONFIG}/inc src/esp/espFramework.c

${CC} -c -o ${CONFIG}/obj/espHandler.o -Wall -fPIC ${LDFLAGS} -Wshorten-64-to-32 -mtune=generic ${DFLAGS} -I${CONFIG}/inc src/esp/espHandler.c

${CC} -c -o ${CONFIG}/obj/espHtml.o -Wall -fPIC ${LDFLAGS} -Wshorten-64-to-32 -mtune=generic ${DFLAGS} -I${CONFIG}/inc src/esp/espHtml.c

${CC} -c -o ${CONFIG}/obj/espSession.o -Wall -fPIC ${LDFLAGS} -Wshorten-64-to-32 -mtune=generic ${DFLAGS} -I${CONFIG}/inc src/esp/espSession.c

${CC} -c -o ${CONFIG}/obj/espTemplate.o -Wall -fPIC ${LDFLAGS} -Wshorten-64-to-32 -mtune=generic ${DFLAGS} -I${CONFIG}/inc src/esp/espTemplate.c

${CC} -c -o ${CONFIG}/obj/mdb.o -Wall -fPIC ${LDFLAGS} -Wshorten-64-to-32 -mtune=generic ${DFLAGS} -I${CONFIG}/inc src/esp/mdb.c

${CC} -c -o ${CONFIG}/obj/sdb.o -Wall -fPIC ${LDFLAGS} -Wshorten-64-to-32 -mtune=generic ${DFLAGS} -I${CONFIG}/inc src/esp/sdb.c

${CC} -shared -o ${CONFIG}/bin/mod_esp.so ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/edi.o ${CONFIG}/obj/espAbbrev.o ${CONFIG}/obj/espFramework.o ${CONFIG}/obj/espHandler.o ${CONFIG}/obj/espHtml.o ${CONFIG}/obj/espSession.o ${CONFIG}/obj/espTemplate.o ${CONFIG}/obj/mdb.o ${CONFIG}/obj/sdb.o ${LIBS} -lappweb -lhttp -lpam -lmpr -lpcre -lmprssl

rm -rf ${CONFIG}/bin/esp.conf
cp -r src/esp/esp.conf ${CONFIG}/bin/esp.conf

${CC} -c -o ${CONFIG}/obj/cgiHandler.o -Wall -fPIC ${LDFLAGS} -Wshorten-64-to-32 -mtune=generic ${DFLAGS} -I${CONFIG}/inc src/modules/cgiHandler.c

${CC} -shared -o ${CONFIG}/bin/mod_cgi.so ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/cgiHandler.o ${LIBS} -lappweb -lhttp -lpam -lmpr -lpcre -lmprssl

${CC} -c -o ${CONFIG}/obj/cgiProgram.o -Wall -fPIC ${LDFLAGS} -Wshorten-64-to-32 -mtune=generic ${DFLAGS} -I${CONFIG}/inc src/utils/cgiProgram.c

${CC} -o ${CONFIG}/bin/cgiProgram ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/cgiProgram.o ${LIBS} ${LDFLAGS}

rm -rf ${CONFIG}/inc/appwebMonitor.h
cp -r src/server/appwebMonitor.h ${CONFIG}/inc/appwebMonitor.h

${CC} -c -o ${CONFIG}/obj/appweb.o -Wall -fPIC ${LDFLAGS} -Wshorten-64-to-32 -mtune=generic ${DFLAGS} -I${CONFIG}/inc src/server/appweb.c

${CC} -o ${CONFIG}/bin/appweb ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/appweb.o ${LIBS} -lappweb -lhttp -lpam -lmpr -lpcre -lmprssl ${LDFLAGS}

cd test >/dev/null ;\
echo '#!../${CONFIG}/bin/cgiProgram' >cgi-bin/testScript ; chmod +x cgi-bin/testScript ;\
cd - >/dev/null 

cd test >/dev/null ;\
echo "#!`type -p sh`" >web/caching/cache.cgi ;\
echo '' >>web/caching/cache.cgi ;\
echo 'echo HTTP/1.0 200 OK' >>web/caching/cache.cgi ;\
echo 'echo Content-Type: text/plain' >>web/caching/cache.cgi ;\
echo 'date' >>web/caching/cache.cgi ;\
chmod +x web/caching/cache.cgi ;\
cd - >/dev/null 

cd test >/dev/null ;\
echo "#!`type -p sh`" >web/basic/basic.cgi ;\
echo '' >>web/basic/basic.cgi ;\
echo 'echo Content-Type: text/plain' >>web/basic/basic.cgi ;\
echo '/usr/bin/env' >>web/basic/basic.cgi ;\
chmod +x web/basic/basic.cgi ;\
cd - >/dev/null 

cd test >/dev/null ;\
cp ../${CONFIG}/bin/cgiProgram cgi-bin/cgiProgram ;\
cp ../${CONFIG}/bin/cgiProgram cgi-bin/nph-cgiProgram ;\
cp ../${CONFIG}/bin/cgiProgram 'cgi-bin/cgi Program' ;\
cp ../${CONFIG}/bin/cgiProgram web/cgiProgram.cgi ;\
chmod +x cgi-bin/* web/cgiProgram.cgi ;\
cd - >/dev/null 

