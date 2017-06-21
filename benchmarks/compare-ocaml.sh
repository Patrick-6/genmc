#!/bin/bash

source ../scripts/terminal.sh
HERD=herd7
RCMC=../../Ocaml/rcmc

# Check for command line options
if test "$1" = "--fast"
then
    fastrun=1
fi

runtest() {
    dir=$1
    testcase="${dir%.*}"
    herd_out=`"${HERD}" -model rc11.cat "${dir}"`
    rcmc_out=`"${RCMC}" -mo "${testcase##*/}"`
    herd_execs=`echo "${herd_out}" | awk '/Positive/ { print $2 }'`
    rcmc_execs=`echo "${rcmc_out}" | awk '/SAFE/ { print $3 }'`
    color="${GREEN}" && [[ "${herd_execs}" != "${rcmc_execs}" ]]  && color="${RED}"
    printf "| %-32s | %-17s | %-17s |\n" \
 	   "${color}${testcase##*/}${NC}" "${herd_execs}" "${rcmc_execs}"
}

# Print table's header
printline
printf "| %-32s | %-28s | %-28s |\n" \
       "${CYAN}TESTCASE${NC}" "${CYAN}HERD${NC}" "${CYAN}RCMC${NC}"
printline

# Run testcases
for dir in herd/*.litmus
do
    if test -n "${fastrun}"
    then
	case "${dir##*/}" in
	    "big0.litmus"|"small0.litmus"|"small1.litmus") continue;;
	    *)                                                     ;;
	esac
    fi
    runtest "${dir}"
done
printline
