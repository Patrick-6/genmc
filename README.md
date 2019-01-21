GenMC
=====
Generic Model Checking for C Programs
-------------------------------------

---

GenMC is a stateless model checker for C programs that works on the
level of LLVM Intermediate Representation.

Author: Michalis Kokologiannakis.

* [License](#license)
* [Dependencies](#dependencies)
* [Installing](#installing)
* [Usage](#usage)
* [Notes](#notes)

<a name="license">License</a>
-----------------------------

This program is distributed under the GPL, version 2 or (at your option)
later. Please see the LICENSE file for details.

<a name="dependencies">Dependencies</a>
---------------------------------------

In order to use the tool on a Debian-based installation, you will need the
following packages:

		autoconf  automake  clang  libclang-dev  llvm  llvm-dev  libffi-dev
		zlib1g-dev

We have tested the tool with LLVM versions 3.5.0 and 3.8.1.
Most likely, it also works for all those in between.

<a name="installing">Installing</a>
----------------------------------

For a default build issue:

		autoreconf --install
		./configure
		make

This will leave the `genmc` executable in the `src` directory.
You can either run it from there (as in the examples below), or issue
`make install`.

Alternatively, the following following command will build the `genmc`
executable in parallel and will also run a subset of all the tests
that come with the system to see if the system was built correctly or
not:

		make -j ftest

<a name="usage">Usage</a>
-------------------------

* To see a list of available options run:

		./src/genmc --help

* To run a particular testcase run:

		./src/genmc [options] <file>

<a name="notes">Notes</a>
------------------------

For feedback, questions, and bug reports please send an e-mail to
`michalis AT mpi-sws DOT org`.
