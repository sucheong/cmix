This is the binary distribution of C-Mix @R on @H

To install, copy the various files to locations of your preference,
and edit the script in cmix-@R/bindist/@H/bin/cmix
to contain the proper pathnames.

Or, if you are familiar with giving installation directory arguments
to GNUish configure scripts, you can just, e.g.

   cd @H
   ./configure --prefix=/home/me/cmix
   make install

The files
   cmixdist.bin
   configure
   install-sh
   Makefile.in
are used for this automatic installation and can be ignored if you
install C-Mix/II manually

----------

For the use and operation of C-Mix/II we refer to the C-Mix/II manual
which is available separately.

----------

When you compile generating extensions, you must supply include path
options so that the line
  #include <cmix/speclib.h>
finds the file distributed as cmix-@R/bindist/include/cmix/speclib.h
'make install' installs this file as ${prefix}/include/cmix/speclib.h

Another option is to set the environment variable CMIX_SPECLIB_H
when running the cmix binary. If set, the value of this variable is
written out instead of <cmix/speclib.h>. E.g.,

CMIX_SPECLIB_H='"/home/me/cmix/include/cmix/speclib.h"'
export CMIX_SPECLIB_H

----------

In any case, you'll have to remember to link the generating extension with
the library distributed as cmix-@R/bindist/@H/lib/libcmix.a
yourself. 'make install' installs it as ${exec_prefix}/lib/libcmix.a
