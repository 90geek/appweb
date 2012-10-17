/**
    espSession.c - Session data storage

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "esp.h"

#if BIT_PACK_ESP && UNUSED
/********************************** Forwards  *********************************/

static char *makeKey(EspSession *sp, cchar *key);
static char *makeSessionID(HttpConn *conn);
static void manageSession(EspSession *sp, int flags);

/************************************* Code ***********************************/

PUBLIC EspSession *espAllocSession(HttpConn *conn, cchar *id, MprTime lifespan)
{
    EspReq      *req;
    EspSession  *sp;

    mprAssert(conn);
    req = conn->data;
    mprAssert(req);

    if ((sp = mprAllocObj(EspSession, manageSession)) == 0) {
        return 0;
    }
    mprSetName(sp, "session");
    sp->lifespan = lifespan;
    if (id == 0) {
        id = makeSessionID(conn);
    }
    sp->id = sclone(id);
    sp->cache = req->esp->sessionCache;
    return sp;
}


PUBLIC void espDestroySession(EspSession *sp)
{
    mprAssert(sp);
    sp->id = 0;
}


static void manageSession(EspSession *sp, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(sp->id);
        mprMark(sp->cache);
    }
}


PUBLIC EspSession *espCreateSession(HttpConn *conn)
{
    return espGetSession(conn, 1);
}


PUBLIC EspSession *espGetSession(HttpConn *conn, int create)
{
    EspReq      *req;
    char        *id;

    mprAssert(conn);
    req = conn->data;
    mprAssert(req);
    if (req->session || !conn) {
        return req->session;
    }
    id = espGetSessionID(conn);
    if (id || create) {
        req->session = espAllocSession(conn, id, conn->limits->sessionTimeout);
        if (req->session && !id) {
            httpSetCookie(conn, ESP_SESSION, req->session->id, "/", NULL, 0, conn->secure);
        }
    }
    return req->session;
}



PUBLIC MprHash *espGetSessionObj(HttpConn *conn, cchar *key)
{
    cchar   *str;

    if ((str = espGetSessionVar(conn, key, 0)) != 0 && *str) {
        mprAssert(*str == '{');
        return mprDeserialize(str);
    }
    return 0;
}


PUBLIC cchar *espGetSessionVar(HttpConn *conn, cchar *key, cchar *defaultValue)
{
    EspSession  *sp;
    cchar       *result;

    mprAssert(conn);
    mprAssert(key && *key);

    result = 0;
    if ((sp = espGetSession(conn, 0)) != 0) {
        result = mprReadCache(sp->cache, makeKey(sp, key), 0, 0);
    }
    return result ? result : defaultValue;
}


PUBLIC int espSetSessionObj(HttpConn *conn, cchar *key, MprHash *obj)
{
    espSetSessionVar(conn, key, mprSerialize(obj, 0));
    return 0;
}


PUBLIC int espSetSessionVar(HttpConn *conn, cchar *key, cchar *value)
{
    EspSession  *sp;

    mprAssert(conn);
    mprAssert(key && *key);
    mprAssert(value);

    if ((sp = espGetSession(conn, 1)) == 0) {
        return 0;
    }
    if (mprWriteCache(sp->cache, makeKey(sp, key), value, 0, sp->lifespan, 0, MPR_CACHE_SET) == 0) {
        return MPR_ERR_CANT_WRITE;
    }
    return 0;
}


PUBLIC char *espGetSessionID(HttpConn *conn)
{
    EspReq  *req;
    cchar   *cookies, *cookie;
    char    *cp, *value;
    int     quoted;

    mprAssert(conn);
    req = conn->data;
    mprAssert(req);

    if (req->session) {
        return req->session->id;
    }
    if (req->sessionProbed) {
        return 0;
    }
    req->sessionProbed = 1;
    cookies = httpGetCookies(conn);
    for (cookie = cookies; cookie && (value = strstr(cookie, ESP_SESSION)) != 0; cookie = value) {
        value += strlen(ESP_SESSION);
        while (isspace((uchar) *value) || *value == '=') {
            value++;
        }
        quoted = 0;
        if (*value == '"') {
            value++;
            quoted++;
        }
        for (cp = value; *cp; cp++) {
            if (quoted) {
                if (*cp == '"' && cp[-1] != '\\') {
                    break;
                }
            } else {
                if ((*cp == ',' || *cp == ';') && cp[-1] != '\\') {
                    break;
                }
            }
        }
        return snclone(value, cp - value);
    }
    return 0;
}


static char *makeSessionID(HttpConn *conn)
{
    char        idBuf[64];
    static int  nextSession = 0;

    mprAssert(conn);

    /* Thread race here on nextSession++ not critical */
    fmt(idBuf, sizeof(idBuf), "%08x%08x%d", PTOI(conn->data) + PTOI(conn), (int) mprGetTime(), nextSession++);
    return mprGetMD5WithPrefix(idBuf, sizeof(idBuf), "::esp.session::");
}


static char *makeKey(EspSession *sp, cchar *key)
{
    return sfmt("session-%s-%s", sp->id, key);
}

#endif /* BIT_PACK_ESP */
/*
    @copy   default

    Copyright (c) Embedthis Software LLC, 2003-2012. All Rights Reserved.

    This software is distributed under commercial and open source licenses.
    You may use the Embedthis Open Source license or you may acquire a 
    commercial license from Embedthis Software. You agree to be fully bound
    by the terms of either license. Consult the LICENSE.md distributed with
    this software for full details and other copyrights.

    Local variables:
    tab-width: 4
    c-basic-offset: 4
    End:
    vim: sw=4 ts=4 expandtab

    @end
 */
