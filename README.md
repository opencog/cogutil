OpenCog Utilities
=================
[![Build Status](https://travis-ci.org/opencog/cogutils.svg?branch=master)](https://travis-ci.org/opencog/cogutils)

The OpenCog utilities is a miscellaneous collection of C++ utilities
use for typical programming tasks in multiple OpenCog projects.

The main project site is at http://opencog.org

Prerequisites
-------------
To build the OpenCog utilities, the packages listed below are required. With a
few exceptions, most Linux distributions will provide these packages. Users of
Ubuntu 14.04 "Trusty Tahr" may use the dependency installer at scripts/octool.
Users of any version of Linux may use the Dockerfile to quickly build a
container in which OpenCog will be built and run.

###### boost
> C++ utilities package
> http://www.boost.org/ | libboost-dev

###### cmake
> Build management tool; v2.8 or higher recommended.
> http://www.cmake.org/ | cmake

###### cxxtest
> Test framework
> http://cxxtest.sourceforge.net/ | https://launchpad.net/~opencog-dev/+archive/ppa

Optional Prerequisites
----------------------
The following are optional, but are strongly recommended, as they result
in "pretty" stack traces, which result in far more useful and readable
stack traces.  These are requires, and not really optional, if you are
a regular OpenCog developer.

###### binutils BFD library
> The GNU binutils linker-loader, ahem, cough, "Binary File Description".
> http://gnu.org/binutils | binutils-dev
> The linker-loader understands calling conventions.

###### iberty
> The GNU GCC compiler tools libiberty component.
> http://gcc.gnu.org | libiberty-dev
> The GCC compiler, and iberty in particular, know stack traces.

Building CogUtils
-----------------
Perform the following steps at the shell prompt:
```
    cd to project root dir
    mkdir build
    cd build
    cmake ..
    make
```
Libraries will be built into subdirectories within build, mirroring the
structure of the source directory root.


Unit tests
----------
To build and run the unit tests, from the ./build directory enter (after
building opencog as above):
```
    make test
```


Install
-------
After building, you MUST install the utilities!
```
    sudo make install
```
