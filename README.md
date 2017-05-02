RCMC
=====
Rectified C11 Model Checking
----------------------------

---

RCMC is a stateless model checker for C11 programs which works on the
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

		clang, llvm-3.5, llvm-3.5-dev

<a name="installing">Installing</a>
----------------------------------

* Download RCMC's sources or clone this repository:

		git-rts@gitlab.mpi-sws.org:viktor/rcmc.git

* For a default build:

		make

<a name="usage">Usage</a>
-------------------------

* To see a list of available options run:

		./rcmc --help
		
* To run a particular testcase run:

		./rcmc [options] <file>

<a name="features">Features</a>
-------------------------------

TODO

<a name="notes">Notes</a>
------------------------

* Testcases should be compiled with `clang -S -emit-llvm` before running `rcmc`.
* RMWs are not yet supported.
