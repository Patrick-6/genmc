#!/bin/bash

# Helper script containing benchmarks to run, as well as their
# arguments.
#
# This script requires getopt from the util-linux package.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, you can access it online at
# http://www.gnu.org/licenses/gpl-2.0.html.
#
# Author: Michalis Kokologiannakis <mixaskok@gmail.com>

declare -A benchmarks

benchmarks["linuxrwlocks"]="-unroll=3;-DCONFIG_RWLOCK_READERS=0 -DCONFIG_RWLOCK_WRITERS=0 -DCONFIG_RWLOCK_RDWR=2"
benchmarks["barrier"]="-unroll=3;-DNUMREADERS=2, -unroll=4;-DNUMREADERS=3"
benchmarks["ms-queue"]="-unroll=3;-DN=2"
benchmarks["chase-lev"]="NO_UNROLL;"
benchmarks["qspinlock"]="-unroll=3;-I../tests/correct/qspinlock/ -I../tests/correct/mcs_spinlock/ -DN=2"
benchmarks["mcs_spinlock"]="-unroll=3;-DN=2 -I../tests/correct/mcs_spinlock/"
benchmarks["mpmc-queue"]="-unroll=3;-DCONFIG_MPMC_READERS=1 -DCONFIG_MPMC_WRITERS=1"
benchmarks["seqlock"]="NO_UNROLL;-I../tests/correct/seqlock/ -DCONFIG_SEQ_READERS=1 -DCONFIG_SEQ_WRITERS=1"
benchmarks["seqlock-atomic"]="-unroll=3;-I../tests/correct/seqlock-atomic/ -DCONFIG_SEQ_READERS=1 -DCONFIG_SEQ_WRITERS=1"
