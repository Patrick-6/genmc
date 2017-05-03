#!/bin/sh

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

RCMC=../rcmc

runfailure() {
    test_file=$1
    shift
    
    if "${RCMC}" "${test_file}"
    then
	echo -n 'FAILED! Unexpected test success!'
	failure=1
    fi
}

runsuccess() {
    test_file=$1
    shift
    
    if "${RCMC}" "${test_file}"
    then
	:
    else
	echo -n 'FAILED! Unexpected test failure!'
	failure=1
    fi
}

echo '--------------------------------------------------------------------'
echo '--- Preparing to run CORRECT testcases...'
echo '--------------------------------------------------------------------'
for dir in correct/*
do
    echo -n '\tTestcase:' "${dir##*/}... "
    vars=0
    expected=""
    failed=""
    if test -f "${dir}/expected.in"
    then
	expected=$(head -n 1 "${dir}/expected.in")
    fi
    for t in $dir/*.c
    do
	vars=$((vars+1))
	if clang -S -emit-llvm -o "${t%.*}.ll" "${t}"
	then
	    explored=`runsuccess "${t}" 2>&1 | awk '{ print $5 }'`
	    expected="${expected:-${explored}}"
	    if test "${expected}" != "${explored}"
	    then
		failed="${explored}"
	    fi
	    rm -rf "${t%.*}.ll"
	else
	    echo 'Malformed testcase!!! Exiting...'
	    exit 2
	fi
    done
    if test -n "${failed}"
    then
	echo 'FAILED! Inconsistent results:'
	echo "\t\t${failed}" 'executions were explored, instead of' \
	     "${expected}" 'that were expected!'
    else
	echo 'Successful (Explored' "${expected}" 'executions in all' \
	     "${vars}" 'variations)'
    fi
done

if test -n "$failure"
then
    echo '--------------------------------------------------------------------'
    echo '!!! ' UNEXPECTED VERIFICATION RESULTS ' !!!'
    echo '--------------------------------------------------------------------'
    exit 1
else
    echo '--------------------------------------------------------------------'
    echo '--- ' Testing proceeded as expected
    echo '--------------------------------------------------------------------'
    exit 0
fi
