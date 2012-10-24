#
#   appweb-macosx.sh -- Build It Shell Script to build Embedthis Appweb
#

ARCH="x64"
ARCH="$(shell uname -m | sed 's/i.86/x86/;s/x86_64/x64/')"
OS="macosx"
PROFILE="debug"
CONFIG="${OS}-${ARCH}-${PROFILE}"
CC="/usr/bin/clang"
LD="/usr/bin/ld"
CFLAGS="-Wall -Wno-deprecated-declarations -g -Wno-unused-result -Wshorten-64-to-32"
DFLAGS="-DBIT_DEBUG"
IFLAGS="-I${CONFIG}/inc"
LDFLAGS="-Wl,-rpath,@executable_path/ -Wl,-rpath,@loader_path/ -g"
LIBPATHS="-L${CONFIG}/bin"
LIBS="-lpthread -lm -ldl"

[ ! -x ${CONFIG}/inc ] && mkdir -p ${CONFIG}/inc ${CONFIG}/obj ${CONFIG}/lib ${CONFIG}/bin

[ ! -f ${CONFIG}/inc/bit.h ] && cp projects/appweb-${OS}-bit.h ${CONFIG}/inc/bit.h
if ! diff ${CONFIG}/inc/bit.h projects/appweb-${OS}-bit.h >/dev/null ; then
	cp projects/appweb-${OS}-bit.h ${CONFIG}/inc/bit.h
fi

rm -rf ${CONFIG}/inc/mpr.h
cp -r src/deps/mpr/mpr.h ${CONFIG}/inc/mpr.h

${CC} -c -o ${CONFIG}/obj/mprLib.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/deps/mpr/mprLib.c

${CC} -dynamiclib -o ${CONFIG}/bin/libmpr.dylib -arch x86_64 ${LDFLAGS} -compatibility_version 4.1.0 -current_version 4.1.0 ${LIBPATHS} -install_name @rpath/libmpr.dylib ${CONFIG}/obj/mprLib.o ${LIBS}

${CC} -c -o ${CONFIG}/obj/mprSsl.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc -I../packages-macosx-x64/openssl/openssl-1.0.1c/include src/deps/mpr/mprSsl.c

${CC} -dynamiclib -o ${CONFIG}/bin/libmprssl.dylib -arch x86_64 ${LDFLAGS} -compatibility_version 4.1.0 -current_version 4.1.0 ${LIBPATHS} -L../packages-macosx-x64/openssl/openssl-1.0.1c -install_name @rpath/libmprssl.dylib ${CONFIG}/obj/mprSsl.o -lmpr ${LIBS} -lssl -lcrypto

${CC} -c -o ${CONFIG}/obj/manager.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/deps/mpr/manager.c

${CC} -o ${CONFIG}/bin/appman -arch x86_64 ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/manager.o -lmpr ${LIBS}

${CC} -c -o ${CONFIG}/obj/makerom.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/deps/mpr/makerom.c

${CC} -o ${CONFIG}/bin/makerom -arch x86_64 ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/makerom.o -lmpr ${LIBS}

rm -rf ${CONFIG}/inc/pcre.h
cp -r src/deps/pcre/pcre.h ${CONFIG}/inc/pcre.h

${CC} -c -o ${CONFIG}/obj/pcre.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/deps/pcre/pcre.c

${CC} -dynamiclib -o ${CONFIG}/bin/libpcre.dylib -arch x86_64 ${LDFLAGS} ${LIBPATHS} -install_name @rpath/libpcre.dylib ${CONFIG}/obj/pcre.o ${LIBS}

rm -rf ${CONFIG}/inc/http.h
cp -r src/deps/http/http.h ${CONFIG}/inc/http.h

${CC} -c -o ${CONFIG}/obj/httpLib.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/deps/http/httpLib.c

${CC} -dynamiclib -o ${CONFIG}/bin/libhttp.dylib -arch x86_64 ${LDFLAGS} -compatibility_version 4.1.0 -current_version 4.1.0 ${LIBPATHS} -install_name @rpath/libhttp.dylib ${CONFIG}/obj/httpLib.o -lpcre -lmpr ${LIBS} -lpam

${CC} -c -o ${CONFIG}/obj/http.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/deps/http/http.c

${CC} -o ${CONFIG}/bin/http -arch x86_64 ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/http.o -lhttp ${LIBS} -lpcre -lmpr -lpam

rm -rf ${CONFIG}/inc/sqlite3.h
cp -r src/deps/sqlite/sqlite3.h ${CONFIG}/inc/sqlite3.h

${CC} -c -o ${CONFIG}/obj/sqlite3.o -arch x86_64 -Wno-deprecated-declarations -g -Wno-unused-result -w ${DFLAGS} -I${CONFIG}/inc src/deps/sqlite/sqlite3.c

${CC} -dynamiclib -o ${CONFIG}/bin/libsqlite3.dylib -arch x86_64 ${LDFLAGS} -compatibility_version 4.1.0 -current_version 4.1.0 ${LIBPATHS} -install_name @rpath/libsqlite3.dylib ${CONFIG}/obj/sqlite3.o ${LIBS}

${CC} -c -o ${CONFIG}/obj/sqlite.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/deps/sqlite/sqlite.c

${CC} -o ${CONFIG}/bin/sqlite -arch x86_64 ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/sqlite.o -lsqlite3 ${LIBS}

rm -rf ${CONFIG}/inc/appweb.h
cp -r src/appweb.h ${CONFIG}/inc/appweb.h

rm -rf ${CONFIG}/inc/customize.h
cp -r src/customize.h ${CONFIG}/inc/customize.h

${CC} -c -o ${CONFIG}/obj/config.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/config.c

${CC} -c -o ${CONFIG}/obj/convenience.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/convenience.c

${CC} -c -o ${CONFIG}/obj/dirHandler.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/dirHandler.c

${CC} -c -o ${CONFIG}/obj/fileHandler.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/fileHandler.c

${CC} -c -o ${CONFIG}/obj/log.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/log.c

${CC} -c -o ${CONFIG}/obj/server.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/server.c

${CC} -dynamiclib -o ${CONFIG}/bin/libappweb.dylib -arch x86_64 ${LDFLAGS} -compatibility_version 4.1.0 -current_version 4.1.0 ${LIBPATHS} -install_name @rpath/libappweb.dylib ${CONFIG}/obj/config.o ${CONFIG}/obj/convenience.o ${CONFIG}/obj/dirHandler.o ${CONFIG}/obj/fileHandler.o ${CONFIG}/obj/log.o ${CONFIG}/obj/server.o -lhttp ${LIBS} -lpcre -lmpr -lpam

rm -rf ${CONFIG}/inc/edi.h
cp -r src/esp/edi.h ${CONFIG}/inc/edi.h

rm -rf ${CONFIG}/inc/esp-app.h
cp -r src/esp/esp-app.h ${CONFIG}/inc/esp-app.h

rm -rf ${CONFIG}/inc/esp.h
cp -r src/esp/esp.h ${CONFIG}/inc/esp.h

rm -rf ${CONFIG}/inc/mdb.h
cp -r src/esp/mdb.h ${CONFIG}/inc/mdb.h

${CC} -c -o ${CONFIG}/obj/edi.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/esp/edi.c

${CC} -c -o ${CONFIG}/obj/espAbbrev.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/esp/espAbbrev.c

${CC} -c -o ${CONFIG}/obj/espFramework.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/esp/espFramework.c

${CC} -c -o ${CONFIG}/obj/espHandler.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/esp/espHandler.c

${CC} -c -o ${CONFIG}/obj/espHtml.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/esp/espHtml.c

${CC} -c -o ${CONFIG}/obj/espSession.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/esp/espSession.c

${CC} -c -o ${CONFIG}/obj/espTemplate.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/esp/espTemplate.c

${CC} -c -o ${CONFIG}/obj/mdb.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/esp/mdb.c

${CC} -c -o ${CONFIG}/obj/sdb.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/esp/sdb.c

${CC} -dynamiclib -o ${CONFIG}/bin/libmod_esp.dylib -arch x86_64 ${LDFLAGS} -compatibility_version 4.1.0 -current_version 4.1.0 ${LIBPATHS} -install_name @rpath/libmod_esp.dylib ${CONFIG}/obj/edi.o ${CONFIG}/obj/espAbbrev.o ${CONFIG}/obj/espFramework.o ${CONFIG}/obj/espHandler.o ${CONFIG}/obj/espHtml.o ${CONFIG}/obj/espSession.o ${CONFIG}/obj/espTemplate.o ${CONFIG}/obj/mdb.o ${CONFIG}/obj/sdb.o -lappweb ${LIBS} -lhttp -lpcre -lmpr -lpam

${CC} -c -o ${CONFIG}/obj/esp.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/esp/esp.c

${CC} -o ${CONFIG}/bin/esp -arch x86_64 ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/edi.o ${CONFIG}/obj/esp.o ${CONFIG}/obj/espAbbrev.o ${CONFIG}/obj/espFramework.o ${CONFIG}/obj/espHandler.o ${CONFIG}/obj/espHtml.o ${CONFIG}/obj/espSession.o ${CONFIG}/obj/espTemplate.o ${CONFIG}/obj/mdb.o ${CONFIG}/obj/sdb.o -lappweb ${LIBS} -lhttp -lpcre -lmpr -lpam

rm -rf ${CONFIG}/bin/esp.conf
cp -r src/esp/esp.conf ${CONFIG}/bin/esp.conf

rm -rf ${CONFIG}/bin/esp-www
cp -r src/esp/www ${CONFIG}/bin/esp-www

rm -rf ${CONFIG}/bin/esp-appweb.conf
cp -r src/esp/esp-appweb.conf ${CONFIG}/bin/esp-appweb.conf

rm -rf ${CONFIG}/inc/ejs.h
cp -r src/deps/ejs/ejs.h ${CONFIG}/inc/ejs.h

rm -rf ${CONFIG}/inc/ejs.slots.h
cp -r src/deps/ejs/ejs.slots.h ${CONFIG}/inc/ejs.slots.h

rm -rf ${CONFIG}/inc/ejsByteGoto.h
cp -r src/deps/ejs/ejsByteGoto.h ${CONFIG}/inc/ejsByteGoto.h

${CC} -c -o ${CONFIG}/obj/ejsLib.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/deps/ejs/ejsLib.c

${CC} -dynamiclib -o ${CONFIG}/bin/libejs.dylib -arch x86_64 ${LDFLAGS} -compatibility_version 4.1.0 -current_version 4.1.0 ${LIBPATHS} -install_name @rpath/libejs.dylib ${CONFIG}/obj/ejsLib.o -lpcre -lsqlite3 -lhttp ${LIBS} -lpcre -lmpr -lpam

${CC} -c -o ${CONFIG}/obj/ejs.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/deps/ejs/ejs.c

${CC} -o ${CONFIG}/bin/ejs -arch x86_64 ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/ejs.o -lejs ${LIBS} -lpcre -lsqlite3 -lhttp -lmpr -lpam -ledit

${CC} -c -o ${CONFIG}/obj/ejsc.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/deps/ejs/ejsc.c

${CC} -o ${CONFIG}/bin/ejsc -arch x86_64 ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/ejsc.o -lejs ${LIBS} -lpcre -lsqlite3 -lhttp -lmpr -lpam

cd src/deps/ejs >/dev/null ;\
../../../${CONFIG}/bin/ejsc --out ../../../${CONFIG}/bin/ejs.mod --optimize 9 --bind --require null ejs.es ;\
cd - >/dev/null 

${CC} -c -o ${CONFIG}/obj/cgiHandler.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/modules/cgiHandler.c

${CC} -dynamiclib -o ${CONFIG}/bin/libmod_cgi.dylib -arch x86_64 ${LDFLAGS} -compatibility_version 4.1.0 -current_version 4.1.0 ${LIBPATHS} -install_name @rpath/libmod_cgi.dylib ${CONFIG}/obj/cgiHandler.o -lappweb ${LIBS} -lhttp -lpcre -lmpr -lpam

${CC} -c -o ${CONFIG}/obj/sslModule.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/modules/sslModule.c

${CC} -dynamiclib -o ${CONFIG}/bin/libmod_ssl.dylib -arch x86_64 ${LDFLAGS} -compatibility_version 4.1.0 -current_version 4.1.0 ${LIBPATHS} -install_name @rpath/libmod_ssl.dylib ${CONFIG}/obj/sslModule.o -lappweb ${LIBS} -lhttp -lpcre -lmpr -lpam

${CC} -c -o ${CONFIG}/obj/authpass.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/utils/authpass.c

${CC} -o ${CONFIG}/bin/authpass -arch x86_64 ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/authpass.o -lappweb ${LIBS} -lhttp -lpcre -lmpr -lpam

${CC} -c -o ${CONFIG}/obj/cgiProgram.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/utils/cgiProgram.c

${CC} -o ${CONFIG}/bin/cgiProgram -arch x86_64 ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/cgiProgram.o ${LIBS}

${CC} -c -o ${CONFIG}/obj/setConfig.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/utils/setConfig.c

${CC} -o ${CONFIG}/bin/setConfig -arch x86_64 ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/setConfig.o -lmpr ${LIBS}

rm -rf ${CONFIG}/inc/appwebMonitor.h
cp -r src/server/appwebMonitor.h ${CONFIG}/inc/appwebMonitor.h

${CC} -c -o ${CONFIG}/obj/appweb.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc src/server/appweb.c

${CC} -o ${CONFIG}/bin/appweb -arch x86_64 ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/appweb.o -lmod_cgi -lmod_ssl -lmod_esp -lappweb ${LIBS} -lhttp -lpcre -lmpr -lpam

rm -rf ${CONFIG}/inc/testAppweb.h
cp -r test/testAppweb.h ${CONFIG}/inc/testAppweb.h

${CC} -c -o ${CONFIG}/obj/testAppweb.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testAppweb.c

${CC} -c -o ${CONFIG}/obj/testHttp.o -arch x86_64 ${CFLAGS} ${DFLAGS} -I${CONFIG}/inc test/testHttp.c

${CC} -o ${CONFIG}/bin/testAppweb -arch x86_64 ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/testAppweb.o ${CONFIG}/obj/testHttp.o -lappweb ${LIBS} -lhttp -lpcre -lmpr -lpam

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
echo "#!`type -p sh`" >web/auth/basic/basic.cgi ;\
echo '' >>web/auth/basic/basic.cgi ;\
echo 'echo Content-Type: text/plain' >>web/auth/basic/basic.cgi ;\
echo 'echo' >>web/auth/basic/basic.cgi ;\
echo '/usr/bin/env' >>web/auth/basic/basic.cgi ;\
chmod +x web/auth/basic/basic.cgi ;\
cd - >/dev/null 

cd test >/dev/null ;\
cp ../${CONFIG}/bin/cgiProgram cgi-bin/cgiProgram ;\
cp ../${CONFIG}/bin/cgiProgram cgi-bin/nph-cgiProgram ;\
cp ../${CONFIG}/bin/cgiProgram 'cgi-bin/cgi Program' ;\
cp ../${CONFIG}/bin/cgiProgram web/cgiProgram.cgi ;\
chmod +x cgi-bin/* web/cgiProgram.cgi ;\
cd - >/dev/null 

cd test >/dev/null ;\
cp -r ../src/esp/www/files/static/js 'web/js' ;\
cd - >/dev/null 

