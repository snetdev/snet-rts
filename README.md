S-Net Runtime System
====================

This is part of the [S-Net software suite on Github](https://github.com/snetdev).
It requires a working installation of [LPEL](https://github.com/snetdev/lpel)
according to the [How to Build Guide](http://snetdev.github.io/content/howtobuild.html).


Bootstrap
---------

To generate the configure script and related build-tools, run

        ./bootstrap

This requires the presence of autoconf, automake and libtool.


Configure
---------

Look for relevant configuration options in the output of:

>     ./configure --help

Then generate a Makefile with

>     ./configure --with-lpel-includes=$LPEL_PREFIX/include \
>                 --with-lpel-libs=$LPEL_PREFIX/lib \
>                 --prefix=$SNET_PREFIX

where `$LPEL_PREFIX` points to the directory prefix of the LPEL installation
and `$SNET_PREFIX` is your desired S-Net destination prefix.


Building
--------

Build and install the software with 

>     make
>     make install


Runtime
-------

S-Net comes with multiple runtime system flavors and threading layers.
The `streams` runtime system offers a choice of threading layers
`pthread`, `lpel` and `lpel_hrc`. A new runtime system `front`
is designed for high-performance computing, fine-grained concurrency
and highly-dynamic S-Net networks.

A number of example applications are located  in the directory `examples`.
They are run by the script `examples/testperf.sh`.

>     cd examples
>     ./testperf.sh

Support
-------

Visit the [S-Net website ](http://www.snet-home.org/) for more information.
Feel free to contact the S-Net developers at <info@snet-home.org>.

