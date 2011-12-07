Embedthis Appweb 4.X
===

The fast, little web server for embedded applications. 

Licensing
---
Appweb is dual-licensed under a GPLv2 license and commercial licenses are offered by Embedthis Software.
See http://appwebserver.org/downloads/licensing.html for licensing details.

### To read documentation:

  See (http://www.appwebserver.org/products/appweb/doc/product/index.html)

### To build:

    ./configure
    make

Images are built into out/bin and out/lib. The build configuration is saved in out/inc/buildConfig.h.

### To test:

    make test

### To run:

    sudo make -C src/server run

This will run appweb in the src/server directory using the src/server/appweb.conf configuration file.

### To install

    make install

### To create installable binary packages

    make package

Resources
---
    - [Appweb web site](http://appwebserver.org)
    - [Embedthis web site](http://embedthis.com)
    - [Appweb GitHub repository](http://github.com/embedthis/appweb-4)
