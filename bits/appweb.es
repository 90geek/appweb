/*
    Support functions for Embedthis Appweb

    Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

require ejs.tar
require ejs.unix

/*
    Copy binary files to package staging area
    This is run for local and cross platforms. The last platform does the packaging
 */
public function packageBinaryFiles(formats = ['tar', 'native'], minimal = false) {
    let settings = bit.settings
    let bin = bit.dir.pkg.join('bin')
    safeRemove(bit.dir.pkg)
    let vname = settings.product + '-' + settings.version + '-' + settings.buildNumber
    let pkg = bin.join(vname)
    pkg.makeDir()

    let contents = pkg.join('contents')

    let prefixes = bit.prefixes;
    let p = {}
    for (prefix in bit.prefixes) {
        p[prefix] = Path(contents.portable.name + bit.prefixes[prefix].removeDrive().portable)
        p[prefix].makeDir()
    }
    let strip = bit.platform.profile == 'debug'

    if (!bit.cross) {
        /* These three files are replicated outside the data directory */
        if (!minimal) {
            install('doc/product/README.TXT', pkg, {fold: true, expand: true})
            install('package/install.sh', pkg.join('install'), {permissions: 0755, expand: true})
            install('package/uninstall.sh', pkg.join('uninstall'), {permissions: 0755, expand: true})
            if (bit.platform.os == 'windows') {
                install('package/windows/LICENSE.TXT', bin, {fold: true, expand: true})
            }
            install(['doc/licenses/*.txt'], p.product.join('LICENSE.TXT'), {
                cat: true,
                textfile: true,
                fold: true,
                title: bit.settings.title + ' Licenses',
            })
            install('doc/product/README.TXT', p.product, {fold: true, expand: true})
            install('package/uninstall.sh', p.bin.join('uninstall'), {permissions: 0755, expand: true})
            install('package/linkup', p.bin, {permissions: 0755})
        }
        install('src/server/web', p.web, {exclude: /mgmt/, subtree: true})
        if (!minimal) {
            install('src/server/web/test/*', p.web.join('test'), {
                include: /.cgi|test.pl|test.py/,
                permissions: 0755,
            })
        }
        install('src/server/mime.types', p.config)
        if (bit.packs.php && bit.packs.php.enable) {
            install('src/server/php.ini', p.config)
        }
        if (Path('src/server/appweb.local').exists) {
            install('src/server/appweb.local', p.config.join('appweb.conf'))
        } else {
            install('src/server/appweb.conf', p.config)
        }
        let conf = Path(contents.portable + '' + bit.prefixes.config.removeDrive().portable + '/appweb.conf')
        let user = getWebUser(), group = getWebUser()
        if (bit.platform.os == 'windows') {
            run(['setConfig', '--home', '.', '--documents', 'web', '--logs', 'logs', '--port', 80,
                '--ssl', 443, '--cache', 'cache', '--modules', 'bin', conf])
        } else {
            run(['setConfig', '--home', prefixes.config, '--documents', prefixes.web, '--logs', prefixes.log,
                '--port', 80, '--ssl', 443, '--user', user, '--group', group,
                '--cache', prefixes.spool.join('cache'), '--modules', prefixes.bin, conf])
        }
        bit.dir.cfg.join('appweb.conf.bak').remove()
        p.spool.join('cache').makeDir({user: user, group: group})
        let tmp = p.log.join('error.log')
        tmp.write()
        tmp.setAttributes({permissions: 0755, user: user, group: group})
    }
    install(bit.dir.bin + '/*', p.bin, {
        include: /appweb|appman|esp|http|auth|makerom|libappweb|libmpr|setConfig|\.dll/,
        exclude: /\.pdb|\.exp|\.lib|\.def|\.suo|\.old/,
        permissions: 0755, 
    })
    if (bit.platform.os != 'windows') {
        install(bit.dir.bin.join('*'), p.bin, {
            permissions: 0755, 
            exclude: /file-save|www|simple|sample/,
        })
    }
    install(bit.dir.bin + '/esp.conf', p.bin)
    install(bit.dir.bin + '/esp.conf', p.config)
    install(bit.dir.bin + '/esp-www', p.bin)
    install(bit.dir.bin + '/esp-appweb.conf', p.bin)
    install(bit.dir.bin.join('http-ca.crt'), p.bin)
    if (!minimal) {
        install(bit.dir.inc.join('*.h'), p.inc)
    }

    if (bit.ssl && bit.platform.os == 'linux') {
        install(bit.dir.bin.join('*.' + bit.ext.shobj + '*'), p.bin, {strip: strip, permissions: 0755})
        for each (f in p.bin.files('*.so.*')) {
            let withver = f.basename
            let nover = withver.name.replace(/\.[0-9]*.*/, '.so')
            let link = p.bin.join(nover)
            f.remove()
            f.symlink(link.basename)
        }
    }
    if (bit.packs.ejscript.enable) {
        p.config.join('ejsrc').write('{ dirs: { cache: "' + bit.prefixes.spool.join('cache') + '" } }\n')
        install(bit.dir.bin.join('ejs*.mod'), p.bin);
    }
    if (!bit.cross) {
        if (bit.platform.os == 'macosx') {
            let daemons = contents.join('Library/LaunchDaemons')
            daemons.makeDir()
            install('package/MACOSX/com.embedthis.appweb.plist', daemons, {permissions: 0644, expand: true})

        } else if (bit.platform.os == 'linux') {
            install('package/LINUX/' + settings.product + '.init', 
                contents.join('etc/init.d', settings.product), 
                {permissions: 0755, expand: true})
            install('package/LINUX/' + settings.product + '.upstart', 
                contents.join('init', settings.product).joinExt('conf'),
                {permissions: 0644, expand: true})

        } else if (bit.platform.os == 'windows') {
            let version = bit.packs.compiler.version.replace('.', '')
            if (bit.platform.arch == 'x64') {
                install(bit.packs.compiler.dir.join('VC/redist/x64/Microsoft.VC' + 
                    version + '.CRT/msvcr' + version + '.dll'), p.bin)
            } else {
                install(bit.packs.compiler.dir.join('VC/redist/x86/Microsoft.VC' + 
                    version + '.CRT/msvcr' + version + '.dll'), p.bin)
            }
            /*
                install(bit.packs.compiler.path.join('../../lib/msvcrt.lib'), p.bin)
             */
            if (!minimal) {
                install(bit.dir.bin.join('removeFiles' + bit.globals.EXE), p.bin)
            }
            install(bit.dir.bin.join('setConf*'), p.bin)
        }
        if (bit.platform.like == 'posix' && !minimal) {
            install('doc/man/*.1', p.productver.join('doc/man/man1'), {compress: true})
        }
    }
    if (!minimal) {
        let files = contents.files('**', {exclude: /\/$/, relative: true})
        files = files.map(function(f) Path("/" + f))
        p.productver.join('files.log').append(files.join('\n') + '\n')
    }
    if (formats && bit.platform.last) {
        package(bit.dir.pkg.join('bin'), formats)
    }
}

public function packageSourceFiles() {
    if (bit.cross) {
        return
    }
    let s = bit.settings
    let src = bit.dir.pkg.join('src')
    let pkg = src.join(s.product + '-' + s.version)
    safeRemove(pkg)
    pkg.makeDir()
    install(['Makefile', 'start.bit', 'main.bit'], pkg)
    install('bits', pkg)
    install('*.md', pkg, {fold: true, expand: true})
    install('configure', pkg, {permissions: 0755})
    install('src', pkg, {
        exclude: /\.log$|\.lst$|ejs\/|ejs.zip|\.stackdump$|\/cache\/|huge.txt|\.swp$|\.tmp/,
    })
    install('test', pkg, {
        exclude: /\.log$|\.lst$|ejs.zip|\.stackdump$|\/cache\/|huge.txt|\.swp$|\.tmp|\.o$|\.obj$|\.so$|\.dylib$|cache\.cgi|basic\.cgi|testScript/,
    })
    install('doc', pkg, {
        exclude: /\/xml\/|\/html\/|Archive|\.mod$|\.so$|\.dylib$|\.o$/,
    })
    install('projects', pkg, {
        exclude: /\/Debug\/|\/Release\/|\.ncb|\.mode1v3|\.pbxuser/,
    })
    install('package', pkg, {})
    package(bit.dir.pkg.join('src'), 'src')
}

public function packageComboFiles() {
    if (bit.cross) {
        return
    }
    let s = bit.settings
    let src = bit.dir.pkg.join('src')
    let pkg = src.join(s.product + '-' + s.version)
    safeRemove(pkg)
    pkg.makeDir()
    install('projects/appweb-' + bit.platform.os + '-default-bit.h', pkg.join('src/deps/appweb/bit.h'))
    install('package/start-flat.bit', pkg.join('src/deps/appweb/start.bit'))
    install('package/Makefile-flat', pkg.join('src/deps/appweb/Makefile'))
    install(['src/deps/mpr/mpr.h', 'src/deps/http/http.h', 'src/appweb.h', 'src/server/windows/appwebMonitor.h',
        'src/esp/edi.h', 'src/esp/mdb.h', 'src/esp/esp.h', 'src/deps/pcre/pcre.h'], 
        pkg.join('src/deps/appweb/appweb.h'), {
        cat: true,
        filter: /^#inc.*appweb.*$|^#inc.*mpr.*$|^#inc.*http.*$|^#inc.*customize.*$|^#inc.*edi.*$|^#inc.*mdb.*$|^#inc.*esp.*$/mg,
        title: bit.settings.title + ' Library Source',
    })
    install(['src/deps/**.c'], pkg.join('src/deps/appweb/deps.c'), {
        cat: true,
        filter: /^#inc.*appweb.*$|^#inc.*mpr.*$|^#inc.*http.*$|^#inc.*customize.*$|^#inc.*edi.*$|^#inc.*mdb.*$|^#inc.*esp.*$/mg,
        exclude: /pcre|makerom|http\.c|sqlite|manager|ejs/,
        header: '#include \"appweb.h\"',
        title: bit.settings.title + ' Library Source',
    })
    install(['src/**.c'], pkg.join('src/deps/appweb/appwebLib.c'), {
        cat: true,
        filter: /^#inc.*appweb.*$|^#inc.*mpr.*$|^#inc.*http.*$|^#inc.*customize.*$|^#inc.*edi.*$|^#inc.*mdb.*$|^#inc.*esp.*$/mg,
        exclude: /deps|server.appweb.c|esp\.c|ejs|samples|romFiles|pcre|appwebMonitor|sqlite|appman|makerom|utils|test|http\.c|sqlite|manager/,
        header: '#include \"appweb.h\"',
        title: bit.settings.title + ' Library Source',
    })
    install(['src/server/appweb.c'], pkg.join('src/deps/appweb/appweb.c'))
    install(['src/server/appweb.conf'], pkg.join('src/deps/appweb/appweb.conf'))

    let econf = pkg.join('src/deps/appweb/appweb.conf')
    econf.write(econf.readString().replace(/..LIBDIR../, ''))

    //  MOB - better to edit appweb.conf and change LIBDIR/esp.conf to be just esp.conf
    install(['src/esp/esp.conf'], pkg.join('src/deps/appweb/esp.conf'))
    install(['src/deps/pcre/pcre.c', 'src/deps/pcre/pcre.h'], pkg.join('src/deps/appweb'))
    install(['src/deps/sqlite/sqlite3.c', 'src/deps/sqlite/sqlite3.h'], pkg.join('src/deps/sqlite'))
    package(bit.dir.pkg.join('src'), ['combo', 'flat'])
}

public function installBinary() {
    if (Config.OS != 'windows' && App.uid != 0) {
        throw 'Must run as root. Use \"sudo bit install\"'
    }
    packageBinaryFiles(null, true)
    if (!bit.cross) {
        if (bit.platform.os == 'windows') {
            Cmd([bit.dir.bin.join('appwebMonitor' + bit.globals.EXE), '--stop'])
        }
        Cmd([bit.dir.bin.join('appman'), '--continue', 'stop', 'disable', 'uninstall'])
    }
    package(bit.dir.pkg.join('bin'), 'install')
    if (!bit.cross) {
        if (Config.OS != 'windows') {
            createLinks()
            updateLatestLink()
        }
        trace('Start', 'appman install enable start')
        Cmd([bit.prefixes.bin.join('appman' + bit.globals.EXE), 'install', 'enable', 'start'])
        if (bit.platform.os == 'windows') {
            Cmd([bit.prefixes.bin.join('appwebMonitor' + bit.globals.EXE)], {detach: true})
        }
    }
    if (!bit.options.keep) {
        bit.dir.pkg.join('bin').removeAll()
    } else {
        trace('Keep', bit.dir.pkg.join('bin'))
    }
    trace('Complete', bit.settings.title + ' installed')
}

public function uninstallBinary() {
    if (Config.OS != 'windows' && App.uid != 0) {
        throw 'Must run as root. Use \"sudo bit install\"'
    }
    trace('Uninstall', bit.settings.title)                                                     
    if (bit.platform.os == 'windows') {
        Cmd([bit.prefixes.bin.join('appwebMonitor'), '--stop'])
    }
    let appman = Cmd.locate(bit.prefixes.bin.join('appman'))
    if (appman) {
        Cmd([appman, '--continue', 'stop', 'disable', 'uninstall'])
    }
    let fileslog = bit.prefixes.productver.join('files.log')
    if (fileslog.exists) {
        for each (let file: Path in fileslog.readLines()) {
            vtrace('Remove', file)
            file.remove()
        }
    }
    fileslog.remove()
    for each (file in bit.prefixes.log.files('*.log*')) {
        file.remove()
    }
    for each (prefix in bit.prefixes) {
        if (!prefix.name.contains(bit.settings.product)) {
            continue
        }
        for each (dir in prefix.files('**', {include: /\/$/}).sort().reverse()) {
            vtrace('Remove', dir)
            dir.remove()
        }
        vtrace('Remove', prefix)
        prefix.remove()
    }
    updateLatestLink()
}

/*
    Create symlinks for binaries and man pages
 */
public function createLinks() {
    let log = []
    let localbin = Path('/usr/local/bin')
    if (localbin.exists) {
        let programs = ['appman', 'appweb', 'http', 'auth', 'esp']
        let bin = bit.prefixes.bin
        let target: Path
        for each (program in programs) {
            let link = Path(localbin.join(program))
            link.symlink(bin.join(program + bit.globals.EXE))
            log.push(link)
        }
        for each (page in bit.prefixes.productver.join('doc/man').files('**/*.1.gz')) {
            let link = Path('/usr/share/man/man1/' + page.basename)
            link.symlink(page)
            log.push(link)
        }
    }
    if (Config.OS != 'windows') {
        let link = Path('/usr/include').join(bit.settings.product)
        link.symlink(bit.prefixes.inc)
        log.push(link)
        bit.prefixes.productver.join('files.log').append(log.join('\n') + '\n')
    }
}

function updateLatestLink() {
    let latest = bit.prefixes.product.join('latest')
    let version = bit.prefixes.product.files('*', {include: /\d+\.\d+\.\d+/}).sort().pop()
    if (version) {
        latest.symlink(version.basename)
    } else {
        latest.remove()
    }
}

/*
    @copy   default
  
    Copyright (c) Embedthis Software LLC, 2003-2013. All Rights Reserved.
    Copyright (c) Michael O'Brien, 1993-2013. All Rights Reserved.
  
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
