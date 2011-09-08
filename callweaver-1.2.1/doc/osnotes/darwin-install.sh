#!/bin/sh
# vim:tw=80:ts=4
################################################################################ 
                     HOW TO BUILD CALLWEAVER.ORG ON MAC OSX 
     Tested with CallWeaver 1.2 (rel.2119) on iMac PPC 800 and OSX 10.4
################################################################################

# open a terminal (i prefer iTerm rather than terminal)
# or enable ssh and login from a remote host.

########################################################################## BEGIN

# We will install callweaver in /usr/local/callweaver.
# everithing will be contained in that directory.

$ sudo mkdir -p /usr/local/callweaver

# we will download and build everything in our home dir 
# in this newly created dev/ dir

$ mkdir dev
$ cd dev

######################################################################## LIBTIFF

# download libtiff

$ tar zxf tiff-3.8.2.tar.gz
$ cd tiff-3.8.2

$ CFLAGS="-I/usr/local/callweaver/include" LDFLAGS="-L/usr/local/callweaver/lib" \
  ./configure --prefix=/usr/local/callweaver
$ make
$ sudo make install

##################################################################### LIBSPANDSP

# download the latest version of spandsp from
# http://www.soft-switch.org/downloads/spandsp/

$ tar zxf spandsp-something.tar.gz
$ cd spandsp-0.0.4/

$ CFLAGS="-I/usr/local/callweaver/include" LDFLAGS="-L/usr/local/callweaver/lib" \
  ./configure --prefix=/usr/local/callweaver
$ make
$ sudo make install

#################################################################### GNU READLINE

# download GNU readline

$ curl -O ftp://ftp.gnu.org/pub/gnu/readline/readline-5.2.tar.gz
$ tar xvf readline-5.2.tar.gz
$ cd readline-5.2

$ CFLAGS="-I/usr/local/callweaver/include" LDFLAGS="-L/usr/local/callweaver/lib" \
  ./configure --prefix=/usr/local/callweaver
$ make
$ sudo make install

################################################################## CALLWEAVER
# and again:

$ CFLAGS="-I/usr/local/callweaver/include" LDFLAGS="-L/usr/local/callweaver/lib" \
  \
  ./configure \
    --enable-iax-trunking=yes \
    --enable-t38=yes \
    --prefix=/usr/local/callweaver \
    \
    --enable-debug=yes \
    --enable-frame-tracing=yes \
    --enable-debug=yes \
    --enable-debug-scheduler=yes \
    --enable-stack-backtraces=yes \
    --disable-optimization

# The second part, starting from --enable-debug is optional,
# it is intended to find bugs.

$ make

# don't yet try to make install. You need callweaver user and group. (thanks to imyrvold)

$ sudo niutil -list / /groups
$ sudo niutil -create / /groups/callweaver
$ sudo niutil -createprop / /groups/callweaver gid 700
$ sudo niutil -createprop / /groups/callweaver passwd "*"
$ sudo niutil -createprop / /groups/callweaver users
$ sudo niutil -appendprop / /groups/callweaver users callweaver
$ sudo niutil -create / /users/callweaver
$ sudo niutil -createprop / /users/callweaver uid 700
$ sudo niutil -createprop / /users/callweaver realname "CallWeaver"
$ sudo niutil -createprop / /users/callweaver shell "/bin/bash"
$ sudo niutil -createprop / /users/callweaver gid 700
$ sudo niutil -createprop / /users/callweaver passwd "*" 

#** OK! now:

$ sudo make install

################################################################## LAST STEP

# Let's become root

$ sudo -s

# Let's switch to the callweaverbin directory

$ cd /usr/local/callweaver/sbin

# AND FINALLY RUN IT!!

$ ./callweaver 

# AND LET'S CONNECT TO THE CONSOLE

$ ./callweaver -rvvv

# Please note. If the last command does not work, then run again
# using "./callweaver -c -g" and read the console output


##################################################################

Written by Massimo "CtRiX" Cetra
Thanks to Navynet SRL - http://www.navynet.it
