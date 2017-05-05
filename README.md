RCMC
=====
Rectified C/C++1 Model Checking
-------------------------------

---

RCMC is a stateless model checker for C/C++11 programs which works on the
level of LLVM Intermediate Representation. 

Author: Michalis Kokologiannakis.

* [License](#license)
* [Dependencies](#dependencies)
* [Installing](#installing)
* [Usage](#usage)
* [Features](#features)
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

We have tested the tool with LLVM versions 3.5.0 and 3.8.1.
Most likely, it also works for all those in between.

<a name="installing">Installing</a>
----------------------------------

For a default build issue:

		autoreconf --install
		./configure
		make

This will leave the `rcmc` executable in the `src` directory.
You can either run it from there (as in the examples below), move it to
some other place (e.g., in the top-level directory) or create a link to it.

<a name="usage">Usage</a>
-------------------------

* To see a list of available options run:

		./src/rcmc --help
		
* To run a particular testcase run:

		./src/rcmc [options] <file>

<a name="features">Features</a>
-------------------------------

TODO

<a name="notes">Notes</a>
------------------------

* Testcases should be compiled with `clang -S -emit-llvm` before running `rcmc`.
* RMWs are not yet supported.
