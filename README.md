This is an experiment to prove that hand-tuned assembly still has an edge over
C++ (even with full optimization -O3). For this first demonstration, I'm using
the image-processing function of brightness-change. There are also the
beginnings of a box-blur (more info on that comes once it's done).

It works on RGBA-images (8 bit/channel) performing a saturated addition or
subtraction with skipping the alpha-channel.

C++ (1D) - naive C++
C++ (2D) - nested and cache-aware loop
plain x86 - simple port to 64bit x86 assembly
SSSE3 - single-threaded SSSE3 assembly
SEEE3 (mt) - multi-threaded SSSE3 assembly

A brief screencast of the code in action can be seen here:

 * https://www.youtube.com/watch?v=QfB0V8F5Vng

The dependencies are modest:

 * cmake 2.8.12
 * nasm 2.11.08
 * C++-14 compiler (e.g. g++, clang++)
 * Qt 5.4

It compiles and runs under recent Linux-distributions (e.g. Ubuntu 16.04). It
should run with little or no changes under Windows 7 (or more recent) and
MacOS X 10.10.5 ("Yosemite") too. But that has not yet been tested. Feedback and
patches are welcome.
