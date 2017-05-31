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

RCMC=../src/rcmc

BLACK=$(tput setaf 0)
RED=$(tput setaf 1)
GREEN=$(tput setaf 2)
YELLOW=$(tput setaf 3)
LIME_YELLOW=$(tput setaf 190)
POWDER_BLUE=$(tput setaf 153)
BLUE=$(tput setaf 4)
MAGENTA=$(tput setaf 5)
CYAN=$(tput setaf 6)
WHITE=$(tput setaf 7)
BRIGHT=$(tput bold)
NC=$(tput sgr0)
BLINK=$(tput blink)
REVERSE=$(tput smso)
UNDERLINE=$(tput smul)

runfailure() {
    test_file=$1
    shift

    if time -p "${RCMC}" "${test_file}"
    then
	echo -n 'FAILED! Unexpected test success!'
	failure=1
    fi
}

runsuccess() {
    test_file=$1
    test_args=$2
    shift 2

    if time -p "${RCMC}" -- "${test_args}" "${test_file}"
    then
	:
    else
	echo -n 'FAILED! Unexpected test failure!'
	failure=1
    fi
}

runvariants() {
    printf "| %-31s | " "${POWDER_BLUE}${dir##*/}${n}${NC}"
    vars=0
    test_time=0
    failed=""
    for t in $dir/variants/*.c
    do
	vars=$((vars+1))
	output=`runsuccess "${t}" "${test_args}" 2>&1`
	explored=`echo "${output}" | awk '/explored/ { print $5 }'`
	time=`echo "${output}" | awk '/real/ { print $2 }'`
	test_time=`echo "${test_time}+${time}" | bc -l`
	total_time=`echo "scale=2; ${total_time}+${time}" | bc -l`
	expected="${expected:-${explored}}"
	if test "${expected}" != "${explored}"
	then
	    explored_failed="${explored}"
	    failed=1
	fi
    done
    if test -n "${failed}"
    then
	average_time=`echo "scale=2; ${test_time}/${vars}" | bc -l`
	printf "%-20s | %-11s | %-11s | %-11s |\n" \
	       "${RED}FAILED${NC}" "${explored_failed:-0}/${expected}" \
	       "${vars}" "${average_time}"
	failure=1
    else
	average_time=`echo "scale=2; ${test_time}/${vars}" | bc -l`
	printf "%-20s | %-11s | %-11s | %-11s |\n" \
	       "${GREEN}SAFE${NC}" "${expected}" "${vars}" "${average_time}"
    fi
}

total_time=0

runtest() {
    dir=$1
    if test -n "${fastrun}"
    then
	case "${dir##*/}" in
	    "big1"|"big2"|"lastzero") continue;;
	    *)                                ;;
	esac
    fi
    if test -f "${dir}/args.in"
    then
	while read test_args <&3 && read expected <&4; do
	    n="/`echo ${test_args} |
                 awk ' { if (match($0, /-DN=[0-9]+/)) print substr($0, RSTART+4, RLENGTH-4) } '`"
	    runvariants
	done 3<"${dir}/args.in" 4<"${dir}/expected.in"
    else
	test_args=""
	n=""
	expected=`head -n 1 "${dir}/expected.in"`
	runvariants
    fi
}

printline() {
    for _ in {0..75}; do echo -n '-'; done; echo ''
}

runall() {
    # First run the testcases in the correct/ directory
    echo ''; printline
    echo '--- Preparing to run CORRECT testcases...'
    printline; echo ''

    # Print table's header
    printline
    printf "| %-25s | %-20s | %-20s | %-20s | %-20s |\n" \
	   "${CYAN}Testcase${NC}" "${CYAN}Result${NC}" "${CYAN}Executions${NC}" \
	   "${CYAN}Variations${NC}" "${CYAN}Avg. time${NC}"
    printline

    # Run correct testcases
    for dir in correct/*
    do
	runtest "${dir}"
    done
}

# Check for command line options
option=$1
if test "${option}" = "--fast"
then
    fastrun=1
else
    testcase="${option}"
fi

# Run specified testcases (default: all)
if test -n "${testcase}"
then
    printline
    echo '--- Preparing to run testcase' "${testcase##*/}..."
    printline
    if test -d "${testcase}"
    then
	runtest "${testcase}"
    else
	echo "Testcase does not exist!"
	failure=1
    fi
else
   runall
fi

if test -n "$failure"
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
