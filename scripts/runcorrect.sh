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
RCMC=../src/rcmc

# Check to see whether we are called from a parent sript..
total_time="${total_time:-0}"
model="${model:-weakra}"

runvariants() {
    printf "| %-31s | " "${POWDER_BLUE}${dir##*/}${n}${NC}"
    vars=0
    test_time=0
    failure=""
    outcome_failure=""
    unroll="" && [[ -f "${dir}/unroll.in" ]] && unroll="-unroll="`head -1 "${dir}/unroll.in"`
    for t in $dir/variants/*.c
    do
	vars=$((vars+1))
	output=`"${RCMC}" "-model=${model}" "${unroll}" -- "${test_args}" "${t}" 2>&1`
	if test "$?" -ne 0
	then
	    outcome_failure=1
	fi
	explored=`echo "${output}" | awk '/explored/ { print $6 }'`
	time=`echo "${output}" | awk '/time/ { print substr($4, 1, length($4)-1) }'`
	test_time=`echo "${test_time}+${time}" | bc -l`
	total_time=`echo "scale=2; ${total_time}+${time}" | bc -l`
	expected="${expected:-${explored}}"
	if test "${expected}" != "${explored}"
	then
	    explored_failed="${explored}"
	    failure=1
	fi
    done
    average_time=`echo "scale=2; ${test_time}/${vars}" | bc -l`
    if test -n "${outcome_failure}"
    then
	printf "%-15s | %-11s | %-5s | %-11s |\n" \
	       "${RED}ERROR${NC}" "${explored}" "${vars}" "${average_time}"
	result=1
    elif test -n "${failure}"
    then
	printf "%-15s | %-11s | %-5s | %-11s |\n" \
	       "${LIME_YELLOW}FAILED${NC}" "${explored_failed:-0}/${expected}" \
	       "${vars}" "${average_time}"
	result=1
    else
	printf "%-15s | %-11s | %-5s | %-11s |\n" \
	       "${GREEN}SAFE${NC}" "${expected}" "${vars}" "${average_time}"
    fi
}

runtest() {
    dir=$1
    if test -f "${dir}/args.${model}.in"
    then
	while read test_args <&3 && read expected <&4; do
	    n="/`echo ${test_args} |
                 awk ' { if (match($0, /-DN=[0-9]+/)) print substr($0, RSTART+4, RLENGTH-4) } '`"
	    runvariants
	done 3<"${dir}/args.${model}.in" 4<"${dir}/expected.${model}.in"
    else
	test_args=""
	n=""
	expected=`head -n 1 "${dir}/expected.${model}.in"`
	runvariants
    fi
}

# Print table's header
printline
printf "| %-25s | %-10s | %-20s | %-10s | %-20s |\n" \
       "${CYAN}Testcase${NC}" "${CYAN}Result${NC}" "${CYAN}Executions${NC}" \
       "${CYAN}Files${NC}" "${CYAN}Avg. time${NC}"
printline

# Run correct testcases
for dir in ../tests/correct/*
do
    if test -n "${fastrun}"
    then
	case "${dir##*/}" in
	    "big1"|"big2"|"fib_bench"|"lastzero") continue;;
	    *)                                            ;;
	esac
    fi
    runtest "${dir}"
done
printline
