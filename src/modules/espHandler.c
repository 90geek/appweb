/*
    espHandler.c -- Embedded Server Pages (ESP) handler

    Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************** Includes **********************************/

#include    "appweb.h"

#if BLD_FEATURE_ESP
#include    "esp.h"
#include    "edi.h"

/************************************* Local **********************************/
/*
    Singleton ESP control structure
 */
static Esp *esp;

/************************************ Forward *********************************/

static EspRoute *allocEspRoute(HttpRoute *loc);
static int espDbDirective(MaState *state, cchar *key, cchar *value);
static char *getControllerEntry(cchar *controllerName);
static EspRoute *getEspRoute(HttpRoute *route);
static int loadApp(HttpConn *conn, int *updated);
static void manageEsp(Esp *esp, int flags);
static void manageReq(EspReq *req, int flags);
static bool moduleIsStale(HttpConn *conn, cchar *source, cchar *module, int *recompile);
static int  runAction(HttpConn *conn);
static void setRouteDirs(MaState *state, cchar *kind);
static bool unloadModule(cchar *module, MprTime timeout);
static bool viewExists(HttpConn *conn);

/************************************* Code ***********************************/

static void openEsp(HttpQueue *q)
{
    HttpConn    *conn;
    HttpRx      *rx;
    HttpRoute   *route;
    EspRoute    *eroute;
    EspReq      *req;

    conn = q->conn;
    rx = conn->rx;

    if (rx->flags & (HTTP_OPTIONS | HTTP_TRACE)) {
        httpHandleOptionsTrace(q->conn);

    } else {
        if ((req = mprAllocObj(EspReq, manageReq)) == 0) {
            httpMemoryError(conn);
            return;
        }
        eroute = 0;
        for (route = rx->route; route; route = route->parent) {
            if ((eroute = httpGetRouteData(route, ESP_NAME)) != 0) {
                break;
            }
        }
        if (eroute == 0) {
            eroute = allocEspRoute(route);
            return;
        }
        conn->data = req;
        req->esp = esp;
        req->eroute = eroute;
        req->autoFinalize = 1;
    }
    /*
        If unloading a module, this lock will cause a wait here while ESP applications are reloaded
     */
    lock(esp);
    esp->inUse++;
    unlock(esp);
}


static void closeEsp(HttpQueue *q)
{
    lock(esp);
    esp->inUse--;
    mprAssert(esp->inUse >= 0);
    unlock(esp);
}


/************************************ Flash ***********************************/

void espClearFlash(HttpConn *conn)
{
    EspReq      *req;

    req = conn->data;
    req->flash = 0;
}


static void setupFlash(HttpConn *conn)
{
    EspReq      *req;

    req = conn->data;
    if (espGetSession(conn, 0)) {
        req->flash = espGetSessionObj(conn, ESP_FLASH_VAR);
        req->lastFlash = 0;
        if (req->flash) {
            mprAssert(req->flash->fn);
            espSetSessionVar(conn, ESP_FLASH_VAR, "");
            req->lastFlash = mprCloneHash(req->flash);
        } else {
            req->flash = 0;
        }
    }
}


static void finalizeFlash(HttpConn *conn)
{
    EspReq  *req;
    MprKey  *kp, *lp;

    req = conn->data;
    if (req->flash) {
        mprAssert(req->flash->fn);
        if (req->lastFlash) {
            mprAssert(req->lastFlash->fn);
            for (ITERATE_KEYS(req->flash, kp)) {
                for (ITERATE_KEYS(req->lastFlash, lp)) {
                    if (smatch(kp->key, lp->key) && kp->data == lp->data) {
                        mprRemoveKey(req->flash, kp->key);
                    }
                }
            }
        }
        if (mprGetHashLength(req->flash) > 0) {
            espSetSessionObj(conn, ESP_FLASH_VAR, req->flash);
        }
    }
}

/************************************ Flash ***********************************/

static void startEsp(HttpQueue *q)
{
    HttpConn    *conn;
    EspReq      *req;

    conn = q->conn;
    req = conn->data;

    if (req) {
        setupFlash(conn);
        if (!runAction(conn)) {
            return;
        }
        if (req->autoFinalize) {
            if (!conn->responded) {
                espRenderView(conn, 0);
            }
            espFinalize(conn);
        }
        if (espIsFinalized(conn)) {
            finalizeFlash(conn);
        }
    }
}


static int runAction(HttpConn *conn)
{
    MprModule   *mp;
    HttpRx      *rx;
    HttpRoute   *route;
    EspRoute    *eroute;
    EspReq      *req;
    EspAction   *action;
    char        *key;
    int         updated, recompile;

    rx = conn->rx;
    req = conn->data;
    route = rx->route;
    eroute = req->eroute;
    mprAssert(eroute);
    updated = 0;
    recompile = 0;

    if (route->sourceName == 0 || *route->sourceName == '\0') {
        return 1;
    }
    /*
        Expand any form var $tokens. This permits ${controller} and user form data to be used in the controller name
     */
    if (schr(route->sourceName, '$')) {
        req->controllerName = stemplate(route->sourceName, rx->params);
    } else {
        req->controllerName = route->sourceName;
    }
    req->controllerPath = mprJoinPath(eroute->controllersDir, req->controllerName);

    if (eroute->appModuleName) {
        if (!loadApp(conn, &updated)) {
            return 0;
        }
    } else if (eroute->update || !mprLookupModule(req->controllerPath)) {
        req->cacheName = mprGetMD5WithPrefix(req->controllerPath, slen(req->controllerPath), "controller_");
        req->module = mprNormalizePath(sfmt("%s/%s%s", eroute->cacheDir, req->cacheName, BLD_SHOBJ));

        if (!mprPathExists(req->controllerPath, R_OK)) {
            httpError(conn, HTTP_CODE_INTERNAL_SERVER_ERROR, "Can't find controller %s", req->controllerPath);
            return 0;
        }
        lock(req->esp);
        if (moduleIsStale(conn, req->controllerPath, req->module, &recompile)) {
            /*  WARNING: GC yield here */
            if (recompile && !espCompile(conn, req->controllerPath, req->module, req->cacheName, 0)) {
                unlock(req->esp);
                return 0;
            }
        }
        if (mprLookupModule(req->controllerPath) == 0) {
            req->entry = getControllerEntry(req->controllerName);
            if ((mp = mprCreateModule(req->controllerPath, req->module, req->entry, eroute)) == 0) {
                unlock(req->esp);
                httpMemoryError(conn);
                return 0;
            }
            mprSetThreadData(esp->local, conn);
            if (mprLoadModule(mp) < 0) {
                unlock(req->esp);
                httpError(conn, HTTP_CODE_INTERNAL_SERVER_ERROR, 
                    "Can't load compiled esp module for %s", req->controllerPath);
                return 0;
            }
            updated = 1;
        }
        unlock(req->esp);
    }
    key = mprJoinPath(eroute->controllersDir, rx->target);
    if ((action = mprLookupKey(esp->actions, key)) == 0) {
        req->controllerPath = mprJoinPath(eroute->controllersDir, req->controllerName);
        key = sfmt("%s/missing", mprGetPathDir(req->controllerPath));
        if ((action = mprLookupKey(esp->actions, key)) == 0) {
            if (!viewExists(conn)) {
                if ((action = mprLookupKey(esp->actions, "missing")) == 0) {
                    httpError(conn, HTTP_CODE_INTERNAL_SERVER_ERROR, "Missing action for %s", rx->target);
                    return 0;
                }
            }
        }
    }
    req->action = action;
    
    if (rx->flags & HTTP_POST) {
        if (!espCheckSecurityToken(conn)) {
            return 1;
        }
    }
    if (action && action->actionProc) {
        //  MOB - does this need a lock
        mprSetThreadData(esp->local, conn);
        if (eroute->controllerBase) {
            (eroute->controllerBase)(conn);
        }
        (action->actionProc)(conn);
    }
    return 1;
}


void espRenderView(HttpConn *conn, cchar *name)
{
    MprModule   *mp;
    HttpRx      *rx;
    EspRoute    *eroute;
    EspReq      *req;
    EspViewProc view;
    int         recompile, updated;
    
    rx = conn->rx;
    req = conn->data;
    eroute = req->eroute;
    recompile = updated = 0;
    
    if (name && *name) {
        req->view = mprJoinPath(eroute->viewsDir, name);
        req->source = mprJoinPathExt(req->view, ".esp");

    } else if (req->controllerName) {
        req->view = mprJoinPath(eroute->viewsDir, rx->target);
        req->source = mprJoinPathExt(req->view, ".esp");

    } else {
        httpMapFile(conn, rx->route);
        req->view = conn->tx->filename;
        req->source = req->view;
    }
    if (eroute->appModuleName) {
        if (!loadApp(conn, &updated)) {
            return;
        }
    } else if (eroute->update || !mprLookupModule(req->source)) {
        req->cacheName = mprGetMD5WithPrefix(req->source, slen(req->source), "view_");
        req->module = mprNormalizePath(sfmt("%s/%s%s", eroute->cacheDir, req->cacheName, BLD_SHOBJ));

        if (!mprPathExists(req->source, R_OK)) {
            httpError(conn, HTTP_CODE_NOT_FOUND, "Can't find web page %s", req->source);
            return;
        }
        lock(req->esp);
        if (moduleIsStale(conn, req->source, req->module, &recompile)) {
            /* WARNING: this will allow GC */
            if (recompile && !espCompile(conn, req->source, req->module, req->cacheName, 1)) {
                unlock(req->esp);
                return;
            }
        }
        if (mprLookupModule(req->source) == 0) {
            req->entry = sfmt("esp_%s", req->cacheName);
            //  MOB - who keeps reference to module?
            if ((mp = mprCreateModule(req->source, req->module, req->entry, eroute)) == 0) {
                unlock(req->esp);
                httpMemoryError(conn);
                return;
            }
            //  MOB - this should return an error msg
            mprSetThreadData(esp->local, conn);
            if (mprLoadModule(mp) < 0) {
                unlock(req->esp);
                httpError(conn, HTTP_CODE_INTERNAL_SERVER_ERROR, "Can't load compiled esp module for %s", req->source);
                return;
            }
        }
        unlock(req->esp);
    }
    if ((view = mprLookupKey(esp->views, mprGetPortablePath(req->source))) == 0) {
        httpError(conn, HTTP_CODE_NOT_FOUND, "Can't find defined view for %s", req->view);
        return;
    }
	httpAddHeaderString(conn, "Content-Type", "text/html");

    //  MOB - does this need a lock
    mprSetThreadData(esp->local, conn);
    (view)(conn);
}


/************************************ Support *********************************/

static char *getControllerEntry(cchar *controllerName)
{
    char    *cp, *entry;

    entry = sfmt("esp_controller_%s", mprTrimPathExt(mprGetPathBase(controllerName)));
    for (cp = entry; *cp; cp++) {
        if (!isalnum((int) *cp) && *cp != '_') {
            *cp = '_';
        }
    }
    return entry;
}


/*
    Load the (flat) app module. If modified, unload and reload if required.
 */
static int loadApp(HttpConn *conn, int *updated)
{
    MprModule   *mp;
    EspRoute    *eroute;
    EspReq      *req;
    MprPath     minfo;
    char        *entry;

    req = conn->data;
    if (req->appLoaded) {
        return 1;
    }
    *updated = 0;
    eroute = req->eroute;
    if ((mp = mprLookupModule(eroute->appModuleName)) != 0) {
        if (eroute->update) {
            mprGetPathInfo(mp->path, &minfo);
            if (minfo.valid && mp->modified < minfo.mtime) {
                if (!unloadModule(eroute->appModuleName, 0)) {
                    mprError("Can't unload module %s. Connections still open. Continue using old version.", 
                        eroute->appModuleName);
                    /* Can't unload - so keep using old module */
                    return 1;
                }
                mp = 0;
            }
        }
    }
    if (mp == 0) {
        entry = sfmt("esp_app_%s", eroute->appModuleName);
        if ((mp = mprCreateModule(eroute->appModuleName, eroute->appModulePath, entry, eroute)) == 0) {
            httpError(conn, HTTP_CODE_INTERNAL_SERVER_ERROR, "Can't find module %s", eroute->appModulePath);
            return 0;
        }
        mprSetThreadData(esp->local, conn);
        if (mprLoadModule(mp) < 0) {
            httpError(conn, HTTP_CODE_INTERNAL_SERVER_ERROR, "Can't load compiled esp module for %s", eroute->appModuleName);
            return 0;
        }
        *updated = 1;
    }
    req->appLoaded = 1;
    return 1;
}


static bool moduleIsStale(HttpConn *conn, cchar *source, cchar *module, int *recompile)
{
    MprModule   *mp;
    MprPath     sinfo, minfo;

    *recompile = 0;
    mprGetPathInfo(module, &minfo);
    if (!minfo.valid) {
        *recompile = 1;
        return 1;
    }
    mprGetPathInfo(source, &sinfo);
    /*
        Use >= to ensure we reload. This may cause redundant reloads as mtime has a 1 sec granularity.
     */
    if (sinfo.valid && sinfo.mtime >= minfo.mtime) {
        if ((mp = mprLookupModule(source)) != 0) {
            if (!unloadModule(source, 0)) {
                mprError("Can't unload module %s. Connections still open. Continue using old version.", source);
                return 0;
            }
        }
        *recompile = 1;
        return 1;
    }
    if ((mp = mprLookupModule(source)) != 0) {
        if (minfo.mtime > mp->modified) {
            /* Module file has been updated */
            if (!unloadModule(source, 0)) {
                mprError("Can't unload module %s. Connections still open. Continue using old version.", source);
                return 0;
            }
            return 1;
        }
    }
    /* Loaded module is current */
    return 0;
}


static bool viewExists(HttpConn *conn)
{
    HttpRx      *rx;
    EspRoute    *eroute;
    EspReq      *req;
    cchar       *source;
    
    rx = conn->rx;
    req = conn->data;
    eroute = req->eroute;
    
    source = mprJoinPathExt(mprJoinPath(eroute->viewsDir, rx->target), ".esp");
    return mprPathExists(source, R_OK);
}


static bool unloadModule(cchar *module, MprTime timeout)
{
    MprModule   *mp;
    MprTime     mark;

    /* MOB - should this suspend new requests */
    if ((mp = mprLookupModule(module)) != 0) {
        mark = mprGetTime();
        do {
            lock(esp);
            /* Own request will count as 1 */
            if (esp->inUse <= 1) {
                mprUnloadModule(mp);
                unlock(esp);
                return 1;
            }
            unlock(esp);
            mprSleep(10);
            /* Defaults to 10 secs */
        } while (mprGetRemainingTime(mark, timeout) > 0);
    }
    return 0;
}


/************************************ Esp Route *******************************/

static EspRoute *allocEspRoute(HttpRoute *route)
{
    EspRoute  *eroute;

    if ((eroute = mprAllocObj(EspRoute, espManageEspRoute)) == 0) {
        return 0;
    }
    httpSetRouteData(route, ESP_NAME, eroute);
    eroute->route = route;
#if DEBUG_IDE
    eroute->cacheDir = mprGetAppDir();
#else
    eroute->cacheDir = mprJoinPath(mprGetAppDir(), "../" BLD_LIB_NAME);
#endif
    eroute->dbDir = route->dir;
    eroute->dir = route->dir;
    eroute->controllersDir = eroute->dir;
    eroute->layoutsDir = eroute->dir;
    eroute->viewsDir = eroute->dir;
    eroute->staticDir = eroute->dir;

    /*
        Setup default parameters for $expansion of Http route paths
     */
    httpSetRoutePathVar(route, "CONTROLLERS_DIR", eroute->controllersDir);
    httpSetRoutePathVar(route, "DB_DIR", eroute->dbDir);
    httpSetRoutePathVar(route, "LAYOUTS_DIR", eroute->layoutsDir);
    httpSetRoutePathVar(route, "STATIC_DIR", eroute->staticDir);
    httpSetRoutePathVar(route, "VIEWS_DIR", eroute->viewsDir);

    eroute->lifespan = 0;
    eroute->keepSource = BLD_DEBUG;
#if BLD_DEBUG
	eroute->update = 1;
	eroute->showErrors = 1;
#endif
    return eroute;
}


static EspRoute *cloneEspRoute(EspRoute *parent, HttpRoute *route)
{
    EspRoute      *eroute;
    
    mprAssert(parent);
    mprAssert(route);

    if ((eroute = mprAllocObj(EspRoute, espManageEspRoute)) == 0) {
        return 0;
    }
    httpSetRouteData(route, ESP_NAME, eroute);
    eroute->route = parent->route;
    eroute->searchPath = parent->searchPath;
    eroute->edi = parent->edi;
    eroute->controllerBase = parent->controllerBase;
    eroute->appModuleName = parent->appModuleName;
    eroute->appModulePath = parent->appModulePath;
    eroute->update = parent->update;
    eroute->keepSource = parent->keepSource;
    eroute->showErrors = parent->showErrors;
    eroute->lifespan = parent->lifespan;
    if (parent->compile) {
        eroute->compile = sclone(parent->compile);
    }
    if (parent->link) {
        eroute->link = sclone(parent->link);
    }
    if (parent->env) {
        eroute->env = mprCloneList(parent->env);
    }
    eroute->dir = parent->dir;
    eroute->cacheDir = parent->cacheDir;
    eroute->controllersDir = parent->controllersDir;
    eroute->dbDir = parent->dbDir;
    eroute->layoutsDir = parent->layoutsDir;
    eroute->viewsDir = parent->viewsDir;
    eroute->staticDir = parent->staticDir;
    return eroute;
}


static void setSimpleDirs(EspRoute *eroute)
{
    char    *dir;

    /* Don't set cache dir here - keep inherited value */
    dir = eroute->dir;
    eroute->controllersDir = dir;
    eroute->dbDir = dir;
    eroute->layoutsDir = dir;
    eroute->viewsDir = dir;
    eroute->staticDir = dir;
}


static void setMvcDirs(EspRoute *eroute, HttpRoute *route)
{
    char    *dir;

    dir = route->dir;
    eroute->dir = route->dir;

    eroute->cacheDir = mprJoinPath(dir, "cache");
    httpSetRoutePathVar(route, "CACHE_DIR", eroute->cacheDir);

    eroute->dbDir = mprJoinPath(dir, "db");
    httpSetRoutePathVar(route, "DB_DIR", eroute->dbDir);

    eroute->controllersDir = mprJoinPath(dir, "controllers");
    httpSetRoutePathVar(route, "CONTROLLERS_DIR", eroute->controllersDir);

    eroute->layoutsDir  = mprJoinPath(dir, "layouts");
    httpSetRoutePathVar(route, "LAYOUTS_DIR", eroute->layoutsDir);

    eroute->staticDir = mprJoinPath(dir, "static");
    httpSetRoutePathVar(route, "STATIC_DIR", eroute->staticDir);

    eroute->viewsDir = mprJoinPath(dir, "views");
    httpSetRoutePathVar(route, "VIEWS_DIR", eroute->viewsDir);
}


void espManageEspRoute(EspRoute *eroute, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(eroute->appModuleName);
        mprMark(eroute->appModulePath);
        mprMark(eroute->cacheDir);
        mprMark(eroute->compile);
        mprMark(eroute->controllersDir);
        mprMark(eroute->dbDir);
        mprMark(eroute->edi);
        mprMark(eroute->dir);
        mprMark(eroute->env);
        mprMark(eroute->layoutsDir);
        mprMark(eroute->link);
        mprMark(eroute->searchPath);
        mprMark(eroute->staticDir);
        mprMark(eroute->viewsDir);
    }
}


static void manageReq(EspReq *req, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(req->action);
        mprMark(req->cacheName);
        mprMark(req->commandLine);
        mprMark(req->controllerName);
        mprMark(req->controllerPath);
        mprMark(req->entry);
        mprMark(req->eroute);
        mprMark(req->flash);
        mprMark(req->module);
        mprMark(req->record);
        mprMark(req->route);
        mprMark(req->session);
        mprMark(req->source);
        mprMark(req->view);
    }
}


static void manageEsp(Esp *esp, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(esp->actions);
        mprMark(esp->ediService);
        mprMark(esp->internalOptions);
        mprMark(esp->local);
        mprMark(esp->mutex);
        mprMark(esp->sessionCache);
        mprMark(esp->views);
    }
}


static EspRoute *getEspRoute(HttpRoute *route)
{
    EspRoute    *eroute, *parent;

    eroute = httpGetRouteData(route, ESP_NAME);
    if (route->parent) {
        /*
            If the parent route has the same route data, then force a clone so the parent route does not get modified
         */ 
        parent = httpGetRouteData(route->parent, ESP_NAME);
        if (eroute == parent) {
            eroute = 0;
        }
    }
    if (eroute == 0) {
        if (route->parent && (parent = httpGetRouteData(route->parent, ESP_NAME)) != 0) {
            eroute = cloneEspRoute(parent, route);
        } else {
            eroute = allocEspRoute(route);
        }
    }
    mprAssert(eroute);
    return eroute;
}


static void setRouteDirs(MaState *state, cchar *kind)
{
    EspRoute    *eroute;

    if ((eroute = getEspRoute(state->route)) == 0) {
        return;
    }
    if (smatch(kind, "none")) {
        setSimpleDirs(eroute);

    } else if (smatch(kind, "simple")) {
        setSimpleDirs(eroute);

    } else if (smatch(kind, "mvc") || smatch(kind, "restful")) {
        setMvcDirs(eroute, state->route);
    }
}


/*********************************** Directives *******************************/
/*
    NOTE: This is not a public directive. Internal use only.
    WARNING: this modifies the route prefix and pattern. Only suitable to be used by EspApp

    EspApp Prefix [Dir [RouteSet [Database]]]

        Prefix       appName
        DocumentRoot path
        AddHandler   espHandler
        EspDir       routeSet
        EspRouteSet  routeSet
 */
static int appDirective(MaState *state, cchar *key, cchar *value)
{
    HttpRoute   *route;
    EspRoute    *eroute;
    char        *appName, *path, *routeSet, *database;

    route = state->route;
    if ((eroute = getEspRoute(route)) == 0) {
        return MPR_ERR_MEMORY;
    }
    if (!maTokenize(state, value, "%S ?S ?S ?S", &appName, &path, &routeSet, &database)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if (*appName != '/') {
        mprError("Script name should start with a \"/\"");
        appName = sjoin("/", appName, NULL);
    }
    appName = stemplate(appName, route->pathTokens);
    if (appName == 0 || *appName == '\0' || scmp(appName, "/") == 0) {
        appName = MPR->emptyString;
    } else {
        httpSetRoutePrefix(route, appName);
    }
    if (route->pattern == 0) {
        httpSetRoutePattern(route, sjoin("/", appName, NULL), 0);
    }
    httpSetRouteSource(route, "");
    if (path == 0) {
        path = route->dir;
    }
    if (path == 0) {
        path = sclone(".");
    }
    httpSetRouteDir(route, path);
    eroute->dir = route->dir;
    httpAddRouteHandler(route, "espHandler", "");
    
    /* Must set dirs first before defining route set */
    if (routeSet) {
        setRouteDirs(state, routeSet);
        httpAddRouteSet(state->route, routeSet);
    }
    if (database) {
        if (espDbDirective(state, key, database) < 0) {
            return MPR_ERR_BAD_STATE;
        }
    }
    /* NOTE: this route is not finalized */
    return 0;
}


/*
    EspApp Prefix [Dir [RouteSet [Database]]]
 */
static int espAppDirective(MaState *state, cchar *key, cchar *value)
{
    HttpRoute   *route;
    int         rc;

    if ((route = httpCreateInheritedRoute(state->route)) == 0) {
        return MPR_ERR_MEMORY;
    }
    state = maPushState(state);
    state->route = route;
    rc = appDirective(state, key, value);
    maPopState(state);
    return rc;
}


#if UNUSED
/*
    EspCache lifespan
 */
static int espCacheDirective(MaState *state, cchar *key, cchar *value)
{
    EspRoute    *eroute;

    if ((eroute = getEspRoute(state->route)) == 0) {
        return MPR_ERR_MEMORY;
    }
    eroute->lifespan = (MprTime) stoi(value, 10, NULL);
    return 0;
}
#endif


/*
    EspCompile template
 */
static int espCompileDirective(MaState *state, cchar *key, cchar *value)
{
    EspRoute    *eroute;

    if ((eroute = getEspRoute(state->route)) == 0) {
        return MPR_ERR_MEMORY;
    }
    eroute->compile = sclone(value);
    return 0;
}


/*
    EspDb provider://database
 */
static int espDbDirective(MaState *state, cchar *key, cchar *value)
{
    EspRoute    *eroute;
    char        *provider, *path;
    int         flags;

    if ((eroute = getEspRoute(state->route)) == 0) {
        return MPR_ERR_MEMORY;
    }
    flags = EDI_CREATE | EDI_AUTO_SAVE;
#if UNUSED
    flags |= smatch(options, "auto") ? EDI_AUTO_SAVE : 0;
#endif
    provider = stok(sclone(value), "://", &path);
    if (provider == 0 || path == 0) {
        mprError("Bad database spec '%s'. Use: provider://database", value);
        return MPR_ERR_CANT_OPEN;
    }
    path = mprJoinPath(eroute->dbDir, path);
    if ((eroute->edi = ediOpen(path, provider, flags)) == 0) {
        mprError("Can't open database %s", path);
        return MPR_ERR_CANT_OPEN;
    }
    //  MOB - who closes?
    return 0;
}


/*
    EspDir key path
 */
static int espDirDirective(MaState *state, cchar *key, cchar *value)
{
    EspRoute    *eroute;
    char        *name, *path;

    if ((eroute = getEspRoute(state->route)) == 0) {
        return MPR_ERR_MEMORY;
    }
    if (!maTokenize(state, value, "%S ?S", &name, &path)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if (scmp(name, "mvc") == 0 || scmp(name, "restful") == 0) {
        setMvcDirs(eroute, state->route);
    } else {
        path = stemplate(mprJoinPath(state->host->home, path), state->route->pathTokens);
        if (scmp(name, "cache") == 0) {
            eroute->cacheDir = path;
        } else if (scmp(name, "controllers") == 0) {
            eroute->controllersDir = path;
        } else if (scmp(name, "db") == 0) {
            eroute->dbDir = path;
        } else if (scmp(name, "layouts") == 0) {
            eroute->layoutsDir = path;
        } else if (scmp(name, "static") == 0) {
            eroute->staticDir = path;
        } else if (scmp(name, "views") == 0) {
            eroute->viewsDir = path;
        }
        httpSetRoutePathVar(state->route, name, path);
    }
    return 0;
}


/*
    EspEnv var string
 */
static int espEnvDirective(MaState *state, cchar *key, cchar *value)
{
    EspRoute    *eroute;
    char        *ekey, *evalue;

    if ((eroute = getEspRoute(state->route)) == 0) {
        return MPR_ERR_MEMORY;
    }
    if (!maTokenize(state, value, "%S %S", &ekey, &evalue)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if (eroute->env == 0) {
        eroute->env = mprCreateList(-1, 0);
    }
    evalue = espExpandCommand(evalue, "", "");
#if UNUSED && KEEP
    /*
        This messes up TMP by prepending an existing value
     */
    if ((prior = getenv(ekey)) != 0) {
        mprAddItem(eroute->env, sfmt("%s=%s;%s", ekey, evalue, prior));
    } else {
#endif
    mprAddItem(eroute->env, sfmt("%s=%s", ekey, evalue));
    if (scasematch(ekey, "PATH")) {
        if (eroute->searchPath) {
            eroute->searchPath = sclone(evalue);
        } else {
            eroute->searchPath = sjoin(eroute->searchPath, MPR_SEARCH_SEP, evalue, NULL);
        }
    }
    return 0;
}


/*
    EspKeepSource on|off
 */
static int espKeepSourceDirective(MaState *state, cchar *key, cchar *value)
{
    EspRoute    *eroute;
    bool        on;

    if ((eroute = getEspRoute(state->route)) == 0) {
        return MPR_ERR_MEMORY;
    }
    if (!maTokenize(state, value, "%B", &on)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    eroute->keepSource = on;
    return 0;
}


#if UNUSED
/*
    EspLifespan secs
 */
static int espLifespanDirective(MaState *state, cchar *key, cchar *value)
{
    EspRoute    *eroute;

    if ((eroute = getEspRoute(state->route)) == 0) {
        return MPR_ERR_MEMORY;
    }
    eroute->lifespan = ((MprTime) atoi(value)) * MPR_TICKS_PER_SEC;
    return 0;
}


/*
    EspLimitSessionCache bytes
 */
static int espLimitSessionCacheDirective(MaState *state, cchar *key, cchar *value)
{
    state->limits->sessionCacheSize = stoi(value, 10, 0);
    mprSetCacheLimits(esp->sessionCache, 0, 0, state->limits->sessionCacheSize, 0);
    return 0;
}
#endif


/*
    EspLink template
 */
static int espLinkDirective(MaState *state, cchar *key, cchar *value)
{
    EspRoute    *eroute;

    if ((eroute = getEspRoute(state->route)) == 0) {
        return MPR_ERR_MEMORY;
    }
    eroute->link = sclone(value);
    return 0;
}


/*
    EspLoad name path
 */
static int espLoadDirective(MaState *state, cchar *key, cchar *value)
{
    EspRoute    *eroute;
    char        *name, *path;

    if ((eroute = getEspRoute(state->route)) == 0) {
        return MPR_ERR_MEMORY;
    }
    if (!maTokenize(state, value, "%S %P", &name, &path)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    eroute->appModuleName = sclone(name);
    eroute->appModulePath = sclone(path);
    return 0;
}


/*
    EspResource [resource ...]
 */
static int espResourceDirective(MaState *state, cchar *key, cchar *value)
{
    char    *name, *next;

    if (value == 0 || *value == '\0') {
        httpAddResource(state->route, "{controller}");
    } else {
        name = stok(sclone(value), ", \t\r\n", &next);
        while (name) {
            httpAddResource(state->route, name);
            name = stok(NULL, ", \t\r\n", &next);
        }
    }
    return 0;
}


/*
    EspResourceGroup [resource ...]
 */
static int espResourceGroupDirective(MaState *state, cchar *key, cchar *value)
{
    char    *name, *next;

    if (value == 0 || *value == '\0') {
        httpAddResourceGroup(state->route, "{controller}");
    } else {
        name = stok(sclone(value), ", \t\r\n", &next);
        while (name) {
            httpAddResourceGroup(state->route, name);
            name = stok(NULL, ", \t\r\n", &next);
        }
    }
    return 0;
}


/*
    EspRoute name methods pattern target source
 */
static int espRouteDirective(MaState *state, cchar *key, cchar *value)
{
    char    *name, *methods, *pattern, *target, *source;

    if (!maTokenize(state, value, "%S %S %S %S %S", &name, &methods, &pattern, &target, &source)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    httpDefineRoute(state->route, name, methods, pattern, target, source);
    return 0;
}


/*
    EspRouteSet kind
 */
static int espRouteSetDirective(MaState *state, cchar *key, cchar *value)
{
    EspRoute    *eroute;
    char        *kind;

    if ((eroute = getEspRoute(state->route)) == 0) {
        return MPR_ERR_MEMORY;
    }
    if (!maTokenize(state, value, "%S", &kind)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    httpAddRouteSet(state->route, kind);
    return 0;
}


/*
    EspShowErrors on|off
 */
static int espShowErrorsDirective(MaState *state, cchar *key, cchar *value)
{
    EspRoute    *eroute;
    bool        on;

    if ((eroute = getEspRoute(state->route)) == 0) {
        return MPR_ERR_MEMORY;
    }
    if (!maTokenize(state, value, "%B", &on)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    eroute->showErrors = on;
    return 0;
}


/*
    EspUpdate on|off
 */
static int espUpdateDirective(MaState *state, cchar *key, cchar *value)
{
    EspRoute    *eroute;
    bool        on;

    if ((eroute = getEspRoute(state->route)) == 0) {
        return MPR_ERR_MEMORY;
    }
    if (!maTokenize(state, value, "%B", &on)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    eroute->update = on;
    return 0;
}

/************************************ Init ************************************/

int maEspHandlerInit(Http *http, MprModule *mp)
{
    HttpStage   *handler;
    MaAppweb    *appweb;

    if ((handler = httpCreateHandler(http, "espHandler", 0, NULL)) == 0) {
        return MPR_ERR_CANT_CREATE;
    }
    handler->open = openEsp; 
    handler->close = closeEsp; 
    handler->start = startEsp; 
    if ((esp = mprAllocObj(Esp, manageEsp)) == 0) {
        return MPR_ERR_MEMORY;
    }
    handler->stageData = esp;
    MPR->espService = esp;
    esp->mutex = mprCreateLock();
    esp->local = mprCreateThreadLocal();
    if ((esp->internalOptions = mprCreateHash(-1, MPR_HASH_STATIC_VALUES)) == 0) {
        return 0;
    }
    if ((esp->views = mprCreateHash(-1, MPR_HASH_STATIC_VALUES)) == 0) {
        return 0;
    }
    if ((esp->actions = mprCreateHash(-1, 0)) == 0) {
        return 0;
    }
    if ((esp->sessionCache = mprCreateCache(MPR_CACHE_SHARED)) == 0) {
        return MPR_ERR_MEMORY;
    }
    appweb = httpGetContext(http);
    maAddDirective(appweb, "EspApp", espAppDirective);
    maAddDirective(appweb, "EspCompile", espCompileDirective);
    maAddDirective(appweb, "EspDb", espDbDirective);
    maAddDirective(appweb, "EspDir", espDirDirective);
    maAddDirective(appweb, "EspEnv", espEnvDirective);
    maAddDirective(appweb, "EspKeepSource", espKeepSourceDirective);
#if UNUSED
    maAddDirective(appweb, "EspLifespan", espLifespanDirective);
    maAddDirective(appweb, "EspCache", espCacheDirective);
    maAddDirective(appweb, "EspLimitSessionCache", espLimitSessionCacheDirective);
#endif
    maAddDirective(appweb, "EspLink", espLinkDirective);
    maAddDirective(appweb, "EspLoad", espLoadDirective);
    maAddDirective(appweb, "EspResource", espResourceDirective);
    maAddDirective(appweb, "EspResourceGroup", espResourceGroupDirective);
    maAddDirective(appweb, "EspRoute", espRouteDirective);
    maAddDirective(appweb, "EspRouteSet", espRouteSetDirective);
    maAddDirective(appweb, "EspShowErrors", espShowErrorsDirective);
    maAddDirective(appweb, "EspUpdate", espUpdateDirective);

    if ((esp->ediService = ediCreateService()) == 0) {
        return 0;
    }
#if BLD_FEATURE_MDB
    mdbInit();
#endif
#if BLD_FEATURE_SDB
    sdbInit();
#endif
    return 0;
}


#else /* BLD_FEATURE_ESP */

int maEspHandlerInit(Http *http)
{
    mprNop(0);
    return 0;
}

#endif /* BLD_FEATURE_ESP */
/*
    @copy   default
    
    Copyright (c) Embedthis Software LLC, 2003-2011. All Rights Reserved.
    Copyright (c) Michael O'Brien, 1993-2011. All Rights Reserved.
    
    This software is distributed under commercial and open source licenses.
    You may use the GPL open source license described below or you may acquire 
    a commercial license from Embedthis Software. You agree to be fully bound 
    by the terms of either license. Consult the LICENSE.TXT distributed with 
    this software for full details.
    
    This software is open source; you can redistribute it and/or modify it 
    under the terms of the GNU General Public License as published by the 
    Free Software Foundation; either version 2 of the License, or (at your 
    option) any later version. See the GNU General Public License for more 
    details at: http://embedthis.com/downloads/gplLicense.html
    
    This program is distributed WITHOUT ANY WARRANTY; without even the 
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
    
    This GPL license does NOT permit incorporating this software into 
    proprietary programs. If you are unable to comply with the GPL, you must
    acquire a commercial license to use this software. Commercial licenses 
    for this software and support services are available from Embedthis 
    Software at http://embedthis.com 
    
    Local variables:
    tab-width: 4
    c-basic-offset: 4
    End:
    vim: sw=4 ts=4 expandtab

    @end
 */
