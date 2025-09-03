OpenCog Utilities
=================

[![CircleCI](https://circleci.com/gh/opencog/cogutil.svg?style=svg)](https://circleci.com/gh/opencog/cogutil)

The OpenCog utilities is a miscellaneous collection of C++ utilities
use for typical programming tasks in multiple OpenCog projects.
These include:
* thread-safe queues, stacks and sets
* asynchronous method caller
* thread-safe resource pool
* thread-safe backtrace printing
* high-performance signal-slot
* random tournament selection
* OS portability layers.


The main project site is at http://opencog.org

Prerequisites
-------------
To build the OpenCog utilities, the packages listed below are required.
With a few exceptions, most Linux distributions will provide these
packages. The Docker containers located at
[https://github.com/opencog/docker](https://github.com/opencog/docker)
simplify the entire install and build process, and casual users are
encouraged to try those first.

###### cmake
> Build management tool; v3.12 or higher recommended.
> http://www.cmake.org/ | cmake

###### cxxtest
> Unit test framework
> https://cxxtest.com/ | `apt-get install cxxtest`

Optional Prerequisites
----------------------
The following are optional, but are strongly recommended, as they result
in "pretty" stack traces, which result in far more useful and readable
stack traces.  These are requires, and not really optional, if you are
a regular OpenCog developer.

###### binutils BFD library
> The GNU binutils linker-loader, ahem, cough, "Binary File Description".
> http://gnu.org/s/binutils | binutils-dev
> The linker-loader understands calling conventions.

###### iberty
> The GNU GCC compiler tools libiberty component.
> http://gcc.gnu.org | libiberty-dev
> The GCC compiler, and iberty in particular, know stack traces.

###### doxygen
> Documentation generator under GNU General Public License
> http://www.stack.nl/~dimitri/doxygen/ | doxygen
> Generates code documentation

Building Cogutil
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
    make check
```


Install
-------
After building, you MUST install the utilities!
```
    sudo make install
```
