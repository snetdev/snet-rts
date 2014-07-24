S-Net Runtime System
====================

This software distribution is part of the
[S-Net software suite on Github](https://github.com/snetdev).
It requires a working installation of
[LPEL](https://github.com/snetdev/lpel)
according to the
[How to Build Guide](http://snetdev.github.io/content/howtobuild.html).


Bootstrap
---------

To generate the configure script and related build-tools, run

        ./bootstrap

This requires the availability of the GNU system tools
`autoconf`, `automake` and `libtool`.


Configure
---------

Look for the most relevant configuration options in the output of:

        ./configure --help=short

Then generate a Makefile with at least the following options:

        ./configure --with-lpel-includes=$LPEL_PREFIX/include \
                    --with-lpel-libs=$LPEL_PREFIX/lib \
                    --prefix=$SNET_PREFIX

where `$LPEL_PREFIX` points to the directory prefix of the `LPEL` installation
and `$SNET_PREFIX` is your desired S-Net destination prefix.


Building
--------

Build and install the software with 

        make
        make install


Compiler
--------

To compile S-Net applications the `snetc` compiler is required.
The latest releases are in 
[the releases repository on Github](https://github.com/snetdev/releases).


Runtime
-------

S-Net comes with a choice of runtime system flavors and threading layers.
The `streams` runtime system offers a choice between three threading layers:
`pthread`, `lpel` and `lpel_hrc`. A new runtime system `front`
was designed for high-performance computing, fine-grained concurrency
and highly-dynamic S-Net networks.


Examples
--------

A number of example applications are located  in the directory `examples`.
They are compiled and run by the script `examples/testperf.sh`:

       cd examples
       ./testperf.sh


Documentation
-------------

Complete documentation on the S-Net coordination language
and the use of the S-Net API in applications is available
in the `S-Net Language Report` which can be found on the
[S-Net documentation](http://snet-home.org/?page_id=7) page.


Support
-------

Visit the [S-Net website ](http://www.snet-home.org/) for more information.
Feel free to contact the S-Net developers at <info@snet-home.org>.

