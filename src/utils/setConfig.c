/**
    setConfig.c  -- Appweb set configuration program
  
    Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************* Includes ***********************************/

#include    "mpr.h"

/***************************** Forward Declarations ***************************/

static char *replace(cchar *str, cchar *pattern, cchar *fmt, ...);

/*********************************** Code *************************************/
#if BIT_WIN_LIKE
int APIENTRY WinMain(HINSTANCE inst, HINSTANCE junk, char *command, int junk2) {
    char    **argv;
    cchar   *documents, *home, *logs, *port, *ssl, *argp, *path, *contents, *revised;
    cchar   *user, *group, *cache, *modules, *bak;
    int     argc, err, nextArg;
    static void logHandler(int flags, int level, cchar *msg);

    if (mprCreate(0, NULL, MPR_USER_EVENTS_THREAD) == NULL) {
        exit(1);
    }
    if ((argc = mprMakeArgv(command, &argv, MPR_ARGV_ARGS_ONLY)) < 0) {
        return FALSE;
    }
    mprSetLogHandler(logHandler);
#else
int main(int argc, char **argv) {
    cchar   *documents, *home, *logs, *port, *ssl, *argp, *path, *contents, *revised;
    cchar   *user, *group, *cache, *modules, *bak;
    int     err, nextArg;
    if (mprCreate(argc, argv, MPR_USER_EVENTS_THREAD) == NULL) {
        exit(1);
    }
#endif
    documents = home = port = ssl = logs = user = group = cache = modules = 0;
    for (err = 0, nextArg = 1; nextArg < argc; nextArg++) {
        argp = argv[nextArg];
        if (*argp != '-') {
            break;
        }
        if (smatch(argp, "--documents") && nextArg < argc) {
            documents = argv[++nextArg];
        } else if (smatch(argp, "--home") && nextArg < argc) {
            home = argv[++nextArg];
        } else if (smatch(argp, "--logs") && nextArg < argc) {
            logs = argv[++nextArg];
        } else if (smatch(argp, "--port") && nextArg < argc) {
            port = argv[++nextArg];
        } else if (smatch(argp, "--ssl") && nextArg < argc) {
            ssl = argv[++nextArg];
        } else if (smatch(argp, "--user") && nextArg < argc) {
            user = argv[++nextArg];
        } else if (smatch(argp, "--group") && nextArg < argc) {
            group = argv[++nextArg];
        } else if (smatch(argp, "--cache") && nextArg < argc) {
            cache = argv[++nextArg];
        } else if (smatch(argp, "--modules") && nextArg < argc) {
            modules = argv[++nextArg];
        } else {
            err++;
        }
    }
    if (nextArg != (argc - 1)) {
        err++;
    }
    if (err) {
#if BIT_WIN_LIKE
        mprError("Bad command line:");
#else
        mprError("Bad command line:\n"
            "  Usage: pathConfig [options]\n"
            "  Switches:\n"
            "    --cache dir          # Cache dir\n"
            "    --documents dir      # Static documents directory\n"
            "    --group groupname    # Group name\n"
            "    --home dir           # Server home directory\n"
            "    --logs dir           # Log directory\n"
            "    --modules dir        # moduels dir\n"
            "    --port number        # HTTP port number\n"
            "    --user username      # User name");
#endif
        return 1;
    }
    path = argv[nextArg++];

    if ((contents = mprReadPathContents(path, NULL)) == 0) {
        mprError("Cannot read %s", path);
        return 1;
    }
	if (port) {
	    contents = replace(contents, "Listen 80", "Listen %s", port);
	}
	if (ssl) {
	    contents = replace(contents, "443", ssl);
	}
    if (documents) {
        contents = replace(contents, "Documents", "Documents \"%s\"", documents);
    }
    if (home) {
        contents = replace(contents, "Home", "Home \"%s\"", home);
    }
    if (logs) {
        contents = replace(contents, "ErrorLog", "ErrorLog \"%s\"", mprJoinPath(logs, "error.log"));
        contents = replace(contents, "AccessLog", "AccessLog \"%s\"", mprJoinPath(logs, "access.log"));
    }
    if (user) {
        contents = replace(contents, "UserAccount", "UserAccount %s", user);
    }
    if (group) {
        contents = replace(contents, "GroupAccount", "GroupAccount %s", group);
    }
    if (cache) {
        contents = replace(contents, "EspDir cache", "EspDir cache \"%s\"", cache);
    }
    if (modules) {
        contents = replace(contents, "LoadModulePath", "LoadModulePath \"%s\"", modules);
    }
    revised = mprGetTempPath(mprGetPathDir(path));
    if (mprWritePathContents(revised, contents, -1, 0644) < 0) {
        mprError("Cannot write %s", revised);
    }
	bak = sfmt("%s.bak", path);
	mprDeletePath(bak);
	if (rename(path, bak) < 0) {
        mprError("Cannot save %s to %s: 0x%x", path, bak, mprGetError());
	}
	mprDeletePath(path);
    if (rename(revised, path) < 0) {
        mprError("Cannot rename %s to %s: 0x%x", revised, path, mprGetError());
		rename(bak, path);
    }
    return 0;
}


/*
    This replace will replace the given pattern and the next word on the same line with the given replacement.
 */
static char *replace(cchar *str, cchar *pattern, cchar *fmt, ...)
{
    va_list     args;
    MprBuf      *buf;
    cchar       *s, *replacement;
    ssize       plen;

    va_start(args, fmt);
    replacement = sfmtv(fmt, args);
    buf = mprCreateBuf(-1, -1);
    plen = slen(pattern);
    for (s = str; *s; s++) {
        if (*s == *pattern && sncmp(s, pattern, plen) == 0) {
            mprPutStringToBuf(buf, replacement);
            for (s += plen; *s && isspace((uchar) *s) && *s != '\n'; s++) ;
            for (; *s && !isspace((uchar) *s) && *s != '\n' && *s != '>'; s++) ;
         
        }
#if BIT_WIN_LIKE
        if (*s == '\n' && s[-1] != '\r') {
            mprPutCharToBuf(buf, '\r');
        }
#endif
        mprPutCharToBuf(buf, *s);
    }
    va_end(args);
    mprAddNullToBuf(buf);
    return sclone(mprGetBufStart(buf));
}



#if BIT_WIN_LIKE
/*
    Default log output is just to the console
 */
static void logHandler(int flags, int level, cchar *msg)
{
    mprWriteToOsLog(msg, 0, 0);
    mprEprintf("%s\n", msg);
}
#endif


/*
    @copy   default

    Copyright (c) Embedthis Software LLC, 2003-2013. All Rights Reserved.

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
