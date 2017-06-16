#!/bin/bash

# Driver script for running tests with RCMC.
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

source terminal.sh
RCMC=./src/rcmc

tmp=`clang --version | grep "clang version" | sed 's/[^0-9\.\-]*//g'`
LLVM_VERSION=${tmp%-*}  # remove suffix ending with "-"

total_time=0
result=""

# Check for command line options
if test "$1" = "--fast"
then
    fastrun=1
fi

# First run the testcases in the correct/ directory
echo ''; printline
echo '--- Preparing to run CORRECT testcases...'
printline; echo ''

source runcorrect.sh

# Then, run the testcases in the wrong/ directory
echo ''; printline
echo '--- Preparing to run WRONG testcases...'
printline; echo ''

source runwrong.sh

if test -n "$result"
then
    echo ''; printline
    echo '!!! ' UNEXPECTED TESTING RESULTS ' !!!' Total time: "${total_time}"
    printline
    exit 1
else
    echo ''; printline
    echo '--- ' Testing proceeded as expected. Total time: "${total_time}"
    printline
    exit 0
fi
