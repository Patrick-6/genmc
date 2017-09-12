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
if test "${total_time}" = ""
then
    total_time=0
fi

runvariants() {
    printf "| %-31s | " "${POWDER_BLUE}${dir##*/}${n}${NC}"
    vars=0
    test_time=0
    diff=""
    failure=""
    outcome_failure=""
    checker_args="" && [[ -f "${dir}/rcmc.in" ]] && checker_args=`head -1 "${dir}/rcmc.in"`
    for t in $dir/variants/*.c
    do
	vars=$((vars+1))
	output=`"${RCMC}" -wrc11 -print-error-trace "${checker_args}" -- "${t}" 2>&1`
	if test "$?" -eq 0
	then
	    outcome_failure=1
	fi
	trace=`echo "${output}" | awk '!/status|time/ {print $0 }' > tmp.trace`
	diff=`diff tmp.trace "${t%.*}.trace-${LLVM_VERSION}"`
	if test -n "${diff}"
	then
	    failure=1
	fi
	explored=`echo "${output}" | awk '/explored/ { print $6 }'`
	time=`echo "${output}" | awk '/time/ { print substr($4, 1, length($4)-1) }'`
	test_time=`echo "${test_time}+${time}" | bc -l`
	total_time=`echo "scale=2; ${total_time}+${time}" | bc -l`
	rm tmp.trace
    done
    average_time=`echo "scale=2; ${test_time}/${vars}" | bc -l`
    if test -n "${outcome_failure}"
    then
	printf "%-22s | %-10s | %-13s |\n" \
	       "${RED}BUG NOT FOUND${NC}" "${vars}" "${average_time}"
	result=1
    elif test -n "${failure}"
    then
	printf "%-28s | %-10s | %-13s |\n" \
	       "${LIME_YELLOW}TRACE DIFF${NC}" "${vars}" "${average_time}"
	result=1
    else
	printf "%-22s | %-10s | %-13s |\n" \
	       "${GREEN}BUG FOUND${NC}" "${vars}" "${average_time}"
    fi
}

runtest() {
    dir=$1
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
	#expected=`head -n 1 "${dir}/expected.in"`
	runvariants
    fi
}

# Print table's header
printline
printf "| %-25s | %-22s | %-19s | %-22s |\n" \
       "${CYAN}Testcase${NC}" "${CYAN}Result${NC}" \
       "${CYAN}Files${NC}" "${CYAN}Avg. time${NC}"
printline

# Run wrong testcases
for dir in ../tests/wrong/*
do
    runtest "${dir}"
done
printline
