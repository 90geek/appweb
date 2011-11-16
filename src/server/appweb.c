/**
    appweb.c  -- AppWeb main program

    Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.

    usage: appweb [options] [IpAddr][:port] [documents]
            --config configFile     # Use given config file instead 
            --debugger              # Disable timeouts to make debugging easier
            --home path             # Set the home working directory
            --log logFile:level     # Log to file file at verbosity level
            --name uniqueName       # Name for this instance
            --threads maxThreads    # Set maximum worker threads
            --version               # Output version information
            -v                      # Same as --log stderr:2
 */

/********************************* Includes ***********************************/

#include    "appweb.h"

/********************************** Locals ************************************/
/*
    Global application object. Provides the top level roots of all data objects for the GC.
 */
typedef struct App {
    Mpr         *mpr;
    MaAppweb    *appweb;
    MaServer    *server;
    MprSignal   *traceToggle;
    char        *documents;
    char        *home;
    char        *configFile;
    char        *pathVar;
    int         workers;
} App;

static App *app;

/***************************** Forward Declarations ***************************/

static int changeRoot(cchar *jail);
static int checkEnvironment(cchar *program);
static int findConfigFile();
static void manageApp(App *app, int flags);
static int initialize(cchar *ip, int port);
static void traceHandler(void *ignored, MprSignal *sp);
static void usageError();

#if BLD_UNIX_LIKE
static int  unixSecurityChecks(cchar *program, cchar *home);
#elif BLD_WIN_LIKE
static int writePort(MaServer *server);
static long msgProc(HWND hwnd, uint msg, uint wp, long lp);
#endif

#ifndef BLD_SERVER_ROOT
    #define BLD_SERVER_ROOT mprGetCurrentPath()
#endif
#ifndef BLD_CONFIG_FILE
    #define BLD_CONFIG_FILE NULL
#endif

/*********************************** Code *************************************/

MAIN(appweb, int argc, char **argv)
{
    Mpr     *mpr;
    cchar   *ipAddrPort, *argp, *jail;
    char    *ip;
    int     argind, port;

    ipAddrPort = 0;
    ip = 0;
    jail = 0;
    port = -1;
    argv[0] = "appweb";

    if ((mpr = mprCreate(argc, argv, MPR_USER_EVENTS_THREAD)) == NULL) {
        exit(1);
    }
    if ((app = mprAllocObj(App, manageApp)) == NULL) {
        exit(2);
    }
    mprAddRoot(app);
    mprAddStandardSignals();

    argc = mpr->argc;
    argv = mpr->argv;
    app->mpr = mpr;
    app->workers = -1;
    app->configFile = BLD_CONFIG_FILE;
    app->home = BLD_SERVER_ROOT;
    app->documents = app->home;

#if BLD_FEATURE_ROMFS
    extern MprRomInode romFiles[];
    mprSetRomFileSystem(romFiles);
#endif

    for (argind = 1; argind < argc; argind++) {
        argp = argv[argind];
        if (*argp != '-') {
            break;
        }
        if (strcmp(argp, "--config") == 0 || strcmp(argp, "--conf") == 0) {
            if (argind >= argc) {
                usageError();
            }
            app->configFile = sclone(argv[++argind]);

#if BLD_UNIX_LIKE
        } else if (strcmp(argp, "--chroot") == 0) {
            if (argind >= argc) {
                usageError();
            }
            jail = mprGetAbsPath(argv[++argind]);
#endif

        } else if (strcmp(argp, "--debugger") == 0 || strcmp(argp, "-D") == 0) {
            mprSetDebugMode(1);

        } else if (strcmp(argp, "--home") == 0) {
            if (argind >= argc) {
                usageError();
            }
            app->home = mprGetAbsPath(argv[++argind]);
            if (chdir(app->home) < 0) {
                mprError("%s: Can't change directory to %s", mprGetAppName(), app->home);
                exit(4);
            }

        } else if (strcmp(argp, "--log") == 0 || strcmp(argp, "-l") == 0) {
            if (argind >= argc) {
                usageError();
            }
            mprStartLogging(argv[++argind], 1);
            mprSetCmdlineLogging(1);

        } else if (strcmp(argp, "--name") == 0 || strcmp(argp, "-n") == 0) {
            if (argind >= argc) {
                usageError();
            }
            mprSetAppName(argv[++argind], 0, 0);

        } else if (strcmp(argp, "--threads") == 0) {
            if (argind >= argc) {
                usageError();
            }
            app->workers = atoi(argv[++argind]);

        } else if (strcmp(argp, "--verbose") == 0 || strcmp(argp, "-v") == 0) {
            mprStartLogging("stderr:2", 1);
            mprSetCmdlineLogging(1);

        } else if (strcmp(argp, "--version") == 0 || strcmp(argp, "-V") == 0) {
            mprPrintf("%s %s-%s\n", mprGetAppTitle(), BLD_VERSION, BLD_NUMBER);
            exit(0);

        } else {
            mprError("Unknown switch \"%s\"", argp);
            usageError();
            exit(5);
        }
    }

    if (mprStart() < 0) {
        mprUserError("Can't start MPR for %s", mprGetAppName());
        mprDestroy(MPR_EXIT_DEFAULT);
        return MPR_ERR_CANT_INITIALIZE;
    }
    if (checkEnvironment(argv[0]) < 0) {
        exit(6);
    }
    if (argc > argind) {
        if (argc > (argind + 2)) {
            usageError();
        }
        ipAddrPort = argv[argind++];
        if (argc > argind) {
            app->documents = sclone(argv[argind++]);
        }
        mprParseSocketAddress(ipAddrPort, &ip, &port, HTTP_DEFAULT_PORT);
        
    } else if (findConfigFile() < 0) {
        exit(7);
    }
    if (jail && changeRoot(jail) < 0) {
        exit(8);
    }
    if (initialize(ip, port) < 0) {
        return MPR_ERR_CANT_INITIALIZE;
    }
    if (maStartAppweb(app->appweb) < 0) {
        mprUserError("Can't start HTTP service, exiting.");
        exit(9);
    }
    /*
        Service I/O events until instructed to exit
     */
    while (!mprIsStopping()) {
        mprServiceEvents(-1, 0);
    }
    mprLog(1, "Stopping Appweb ...");
    maStopAppweb(app->appweb);
    mprDestroy(MPR_EXIT_DEFAULT);
    return 0;
}


static void manageApp(App *app, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(app->appweb);
        mprMark(app->server);
        mprMark(app->traceToggle);
        mprMark(app->documents);
        mprMark(app->configFile);
        mprMark(app->pathVar);
        mprMark(app->home);
    }
}


/*
    Move into a chroot jail
 */
static int changeRoot(cchar *jail)
{
#if BLD_UNIX_LIKE
    if (chdir(app->home) < 0) {
        mprError("%s: Can't change directory to %s", mprGetAppName(), app->home);
        return MPR_ERR_CANT_INITIALIZE;
    }
    if (chroot(jail) < 0) {
        if (errno == EPERM) {
            mprError("%s: Must be super user to use the --chroot option", mprGetAppName());
        } else {
            mprError("%s: Can't change change root directory to %s, errno %d", mprGetAppName(), jail, errno);
        }
        return MPR_ERR_CANT_INITIALIZE;
    }
#endif
    return 0;
}


static int initialize(cchar *ip, int port)
{
    if ((app->appweb = maCreateAppweb()) == 0) {
        mprUserError("Can't create HTTP service for %s", mprGetAppName());
        return MPR_ERR_CANT_CREATE;
    }
    MPR->appwebService = app->appweb;

    if ((app->server = maCreateServer(app->appweb, "default")) == 0) {
        mprUserError("Can't create HTTP server for %s", mprGetAppName());
        return MPR_ERR_CANT_CREATE;
    }
    if (maConfigureServer(app->server, app->configFile, app->home, app->documents, ip, port) < 0) {
        /* mprUserError("Can't configure the server, exiting."); */
        return MPR_ERR_CANT_CREATE;
    }
    if (app->workers >= 0) {
        mprSetMaxWorkers(app->workers);
    }
#if BLD_WIN_LIKE
    writePort(app->server);
#else
    app->traceToggle = mprAddSignalHandler(SIGUSR2, traceHandler, 0, 0, MPR_SIGNAL_AFTER);
#endif
    return 0;
}


static int findConfigFile()
{
    cchar   *userPath;

    userPath = app->configFile;
    if (app->configFile == 0) {
        app->configFile = mprJoinPathExt(mprGetAppName(), ".conf");
    }
    if (!mprPathExists(app->configFile, R_OK)) {
        if (!userPath) {
            app->configFile = mprJoinPath(mprGetAppDir(), sfmt("../%s/%s.conf", BLD_LIB_NAME, mprGetAppName()));
        }
        if (!mprPathExists(app->configFile, R_OK)) {
            mprError("Can't open config file %s", app->configFile);
            return MPR_ERR_CANT_OPEN;
        }
    }
    return 0;
}


static void usageError(Mpr *mpr)
{
    cchar   *name;

    name = mprGetAppName();

    mprPrintfError("\n%s Usage:\n\n"
    "  %s [options] [IPaddress][:port] [documents]\n\n"
    "  Options:\n"
    "    --config configFile    # Use named config file instead appweb.conf\n"
    "    --chroot directory     # Change root directory to run more securely (Unix)\n"
    "    --debugger             # Disable timeouts to make debugging easier\n"
    "    --home directory       # Change to directory to run\n"
    "    --log logFile:level    # Log to file file at verbosity level\n"
    "    --name uniqueName      # Unique name for this instance\n"
    "    --threads maxThreads   # Set maximum worker threads\n"
    "    --verbose              # Same as --log stderr:2\n\n"
    "    --version              # Output version information\n\n"
    "  Without IPaddress, %s will read the appweb.conf configuration file.\n\n",
        mprGetAppTitle(), name, name, name, name);
    exit(10);
}


/*
    Security checks. Make sure we are staring with a safe environment
 */
static int checkEnvironment(cchar *program)
{
#if BLD_UNIX_LIKE
    char   *home;
    home = mprGetCurrentPath();
    if (unixSecurityChecks(program, home) < 0) {
        return -1;
    }
    app->pathVar = sjoin("PATH=", getenv("PATH"), ":", mprGetAppDir(), NULL);
    putenv(app->pathVar);
#endif
    return 0;
}


/*
    SIGUSR2 will toggle trace from level 2 to 6
 */
static void traceHandler(void *ignored, MprSignal *sp)
{
    int     level;

    level = mprGetLogLevel() > 2 ? 2 : 6;
    mprLog(0, "Change log level to %d", level);
    mprSetLogLevel(level);
}


#if BLD_UNIX_LIKE
/*
    Security checks. Make sure we are staring with a safe environment
 */
static int unixSecurityChecks(cchar *program, cchar *home)
{
    struct stat     sbuf;

    if (((stat(home, &sbuf)) != 0) || !(S_ISDIR(sbuf.st_mode))) {
        mprUserError("Can't access directory: %s", home);
        return MPR_ERR_BAD_STATE;
    }
    if ((sbuf.st_mode & S_IWOTH) || (sbuf.st_mode & S_IWGRP)) {
        mprUserError("Security risk, directory %s is writeable by others", home);
    }

    /*
        Should always convert the program name into a fully qualified path. Otherwise this fails.
     */
    if (*program == '/') {
        if ((stat(program, &sbuf)) != 0) {
            mprUserError("Can't access program: %s", program);
            return MPR_ERR_BAD_STATE;
        }
        if ((sbuf.st_mode & S_IWOTH) || (sbuf.st_mode & S_IWGRP)) {
            mprUserError("Security risk, Program %s is writeable by others", program);
        }
        if (sbuf.st_mode & S_ISUID) {
            mprUserError("Security risk, %s is setuid", program);
        }
        if (sbuf.st_mode & S_ISGID) {
            mprUserError("Security risk, %s is setgid", program);
        }
    }
    return 0;
}
#endif /* BLD_HOST_UNIX */


#if BLD_WIN_LIKE
/*
    Write the port so the monitor can manage
 */ 
static int writePort(MaServer *server)
{
    HttpHost    *host;
    char        numBuf[16], *path;
    int         fd, len;

    host = mprGetFirstItem(server->http->hosts);
    //  TODO - should really go to a BLD_LOG_DIR
    path = mprJoinPath(mprGetAppDir(), "../.port.log");
    if ((fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0666)) < 0) {
        mprError("Could not create port file %s", path);
        return MPR_ERR_CANT_CREATE;
    }
    mprSprintf(numBuf, sizeof(numBuf), "%d", host->port);

    len = (int) strlen(numBuf);
    numBuf[len++] = '\n';
    if (write(fd, numBuf, len) != len) {
        mprError("Write to file %s failed", path);
        return MPR_ERR_CANT_WRITE;
    }
    close(fd);
    return 0;
}
#endif /* BLD_WIN_LIKE */


#if VXWORKS
/*
    VxWorks link resolution
 */
int _cleanup() {
    return 0;
}
int _exit() {
    return 0;
}

/*
    Create a routine to pull in the GCC support routines for double and int64 manipulations. Do this
    incase any modules reference these routines. Without this, the modules have to reference them. Which leads to
    multiple defines if two modules include them.
 */
double  __dummy_appweb_floating_point_resolution(double a, double b, int64 c, int64 d, uint64 e, uint64 f) {
    /*
        Code to pull in moddi3, udivdi3, umoddi3, etc .
     */
    a = a / b; a = a * b; c = c / d; c = c % d; e = e / f; e = e % f;
    c = (int64) a;
    d = (uint64) a;
    a = (double) c;
    a = (double) e;
    if (a == b) {
        return a;
    } return b;
}

#endif

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
