Embedthis Appweb 4.X
===

The fast, little web server for embedded applications. 

Branches
---
The repository has three branches:

* master - Most recent release of the software.
* lts - Most recent LTS release of the software.
* dev - Current ongoing development. Least stable and may not be suitable for development.
 
Licensing
---
See [LICENSE.md](LICENSE.md) for details.

### To read documentation:

  See https://embedthis.com/appweb/doc/index.html

### Building
    You can build with make or with Bit. Bit is quicker and more flexible.
    To install Bit, download Bit from https://embedthis.com/downloads/bit/download.ejs

### To build with make:

    make

    You can pass make variables to tailor the build. For a list of variables:

	make help

### To build with Bit:

    ./configure
    bit

	For a list of configure options:

	./configure --help

### To run

	make run

    or

    bit run

### To install:

    sudo make install

	or 

    sudo bit install

### To uninstall

    sudo make uninstall

	or 

    sudo bit uninstall

### To test:

    bit test

Resources
---
  - [Appweb web site](http://appwebserver.org/)
  - [Appweb GitHub repository](http://github.com/embedthis/appweb)
  - [Appweb Mailing list](http://groups.google.com/groups/appweb)
  - [Embedthis web site](https://embedthis.com/)
