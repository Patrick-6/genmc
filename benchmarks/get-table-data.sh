#!/bin/bash

# Script that runs experiments and produces the data for a LaTeX
# table.
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

source ../scripts/terminal.sh
RCMC_CPP=../src/rcmc
RCMC_OCAML=../../Ocaml/rcmc
HERD=herd7
NIDHUGG=nidhuggc
CDSCHECKER=$PATHTOCDS/benchmarks/run.sh

declare -a tool_resi
declare -a benchmarks

benchmarks=( "ainc3" "ainc4" "ainc5" "ainc6"
	     "big0"
	     "binc3" "binc4"
	     "casrot3" "casrot4" "casrot5" "casrot6" "casrot8"
	     "casw3" "casw4" "casw5" "casw6"
	     "readers3" "readers8" "readers13"
	     "indexer12" "indexer13" "indexer14" "indexer15"
             "lastzero5" "lastzero10" "lastzero15"
	     "fib_bench3" "fib_bench4" "fib_bench5" )

runherd() {
    dir="$1"
    testcase="${dir%.*}"
    herd_out=`/usr/bin/time -p "${HERD}" -model rc11.cat "${dir}" 2>&1`
    explored=`echo "${herd_out}" | awk '/Positive/ { print $2 }'`
    time=`echo "${herd_out}" | awk '/real/ { print $2 }'`
    result=`printf "%-5s & %-5s" "${explored}" "${time}"`
    if test "${plotmode}" == "y"
    then
	echo "${result}" >> "herd.out"
    else
	tool_res["${herd_col}"]="${result}"
    fi
}

runnidhuggtest() {
    dir="${1%%[[:digit:]]*}" && [[ "${1##*/}" == "CoRR1" || "${1##*/}" == "CoRR2" ||
				   "${1##*/}" == "big0" || "${1##*/}" == "big1" ||
				   "${1##*/}" == "2CoWR" ]] && dir="$1"
    suffix="${1##*[[:alpha:]]}"
    test_args="" && [[ -n "${suffix}" ]] && test_args="-DN=${suffix}"
    model="$2"
    vars=0
    time_total=0
    explored_total=0
    unroll="" && [[ -f "${dir}/unroll.in" ]] && unroll="--unroll="`head -1 "${dir}/unroll.in"`
    for t in "${dir}"/variants/*.c
    do
	vars=$((vars+1))
	output=`nidhuggc "${test_args}" -- --"${model}" "$t" 2>&1`
	explored=`echo "${output}" | awk '/Trace count/ { print $3 }'`
	blocked=`echo "${output}" | awk '/Trace count/ { print $5 }'`
	time=`echo "${output}" | awk '/time/ { print substr($4, 1, length($4)) }'`
	time_total=`echo "${time_total}+${time}" | bc -l`
	explored_total=`echo "${explored_total}+${explored}" | bc -l`
	## explored_total=`echo "${explored_total}+${blocked}" | bc -l`
    done
    average_explored=`echo "scale=0; ${explored_total}/${vars}" | bc -l`
    average_time=`echo "scale=2; ${time_total}/${vars}" | bc -l`
    nidhugg_result=`printf "%-5s & %-5s" "${average_explored}" "${average_time}"`
    nidhugg_result=`printf "%-5s + %-5s & %-5s" "${average_explored}" "${blocked}" "${average_time}"`
    if test "${plotmode}" == "y"
    then
	echo "${nidhugg_result}" >> "nidhugg.${model}.out"
    else
	if test -n "${tool_res[$nid_col]}"
	then
	    tool_res["${nid_col}"]="${tool_res[$nid_col]} & ${nidhugg_result}"
	else
    	    tool_res["${nid_col}"]="${nidhugg_result}"
	fi
    fi
}

runnidhugg() {
    name="$1"
    tool_res["${nid_col}"]=""
    for m in "${run_sc}" "${run_tso}" "${run_pso}"
    do
	if test -z "$m"
	then
	    continue
	fi
	if test "${plotmode}" == "y"
	then
	    runnidhuggtest "../tests/correct/${name}" "$m" &
	else
	    runnidhuggtest "../tests/correct/${name}" "$m"
	fi
    done
}

runcdstest() {
    dir="$1"
    testname=`echo "${dir}" | awk '{ print tolower($0) }'`
    test_args="-m 2 -y" && [[ -f "${dir}/args.in" ]] && test_args=`head -1 "${dir}/args.in"`
    output=`/usr/bin/time -p "${CDSCHECKER}" "${PATHTOCDS}/benchmarks/${dir}/${testname}" \
            "${test_args}" 2>&1`
    explored=`echo "${output}" | awk '/bug-free/ { print $6 }'`
    redundant=`echo "${output}" | awk '/redundant/ { print $5 }'`
    infeasible=`echo "${output}" | awk '/infeasible/ { print $5 }'`
    time=`echo "${output}" | awk '/real/ { print $2 }'`
    result=`printf "%-5s + %-5s + %-5s & %-5s" "${explored}" "${redundant}" "${infeasible}" "${time}"`
    if test "${plotmode}" == "y"
    then
	echo "${result}" >> "cdschecker.out"
    else
	tool_res["${cds_col}"]="${result}"
    fi
}

runcdschecker() {
    name="$1"
    tool_res["${cds_col}"]=""
    runcdstest "${name}"
}

runocaml() {
    name="$1"
    model="$2"
    rcmc_out=`/usr/bin/time -p "${RCMC_OCAML}" "-${model}" "${name}" 2>&1`
    explored=`echo "${rcmc_out}" | awk '/SAFE/ { print $3 }'`
    time=`echo "${rcmc_out}" | awk '/real/ { print $2 }'`
    rcmc_result=`printf "%-5s & %-5s" "${explored}" "${time}"`
    if test "${plotmode}" == "y"
    then
	echo "${rcmc_result}" >> "rcmc.${model}.out"
    else
	if test -n "${tool_res[$rcmc_col]}"
	then
	    tool_res["${rcmc_col}"]="${tool_res[$rcmc_col]} & ${rcmc_result}"
	else
    	    tool_res["${rcmc_col}"]="${rcmc_result}"
	fi
    fi
}

runcpp() {
    dir="${1%%[[:digit:]]*}" && [[ "${1##*/}" == "CoRR1" || "${1##*/}" == "CoRR2" ||
				   "${1##*/}" == "big0" || "${1##*/}" == "big1" ||
				   "${1##*/}" == "2CoWR" ]] && dir="$1"
    suffix="${1##*[[:alpha:]]}"
    test_args="" && [[ -n "${suffix}" ]] && test_args="-DN=${suffix}"
    model="$2"
    vars=0
    time_total=0
    explored_total=0
    unroll="" && [[ -f "${dir}/unroll.in" ]] && unroll="-unroll="`head -1 "${dir}/unroll.in"`
    for t in "${dir}"/variants/*.c
    do
	vars=$((vars+1))
	output=`"${RCMC_CPP}" "-model=${model}" "${unroll}" -- "${test_args}" "${t}" 2>&1`
	explored=`echo "${output}" | awk '/explored/ { print $6 }'`
	time=`echo "${output}" | awk '/time/ { print substr($4, 1, length($4)-1) }'`
	time_total=`echo "${time_total}+${time}" | bc -l`
	explored_total=`echo "${explored_total}+${explored}" | bc -l`
    done
    average_explored=`echo "scale=0; ${explored_total}/${vars}" | bc -l`
    average_time=`echo "scale=2; ${time_total}/${vars}" | bc -l`
    rcmc_result=`printf "%-5s & %-5s" "${average_explored}" "${average_time}"`
    if test "${plotmode}" == "y"
    then
	echo "${rcmc_result}" >> "rcmc.${model}.out"
    else
	if test -n "${tool_res[$rcmc_col]}"
	then
	    tool_res["${rcmc_col}"]="${tool_res[$rcmc_col]} & ${rcmc_result}"
	else
    	    tool_res["${rcmc_col}"]="${rcmc_result}"
	fi
    fi
}

runrcmc() {
    name="$1"
    tool_res["${rcmc_col}"]=""
    for m in "${run_weakra}" "${run_mo}" "${run_wb}"
    do
	if test -z "$m"
	then
	    continue
	fi
	if test "${impl}" == "c++"
	then
	    if test "${plotmode}" == "y"
	    then
		runcpp "../tests/correct/${name}" "$m" &
	    else
		runcpp "../tests/correct/${name}" "$m"
	    fi
	else
	    if test "${plotmode}" == "y"
	    then
		runocaml "${name}" "$m" &
	    else
		runocaml "${name}" "$m"
	    fi
	fi
    done
}

runalltools() {
    dir=$1
    name=$2
    [[ -n "${herd_col}" ]] && runherd "${dir}"
    [[ -n "${nid_col}" ]] && runnidhugg "${name%.*}"
    [[ -n "${cds_col}" ]] && runcdschecker "${name%.*}"
    [[ -n "${rcmc_col}" ]] && runrcmc "${name%.*}"
}

runalltoolsfullypar() {
    dir=$1
    name=$2
    [[ -n "${herd_col}" ]] && runherd "${dir}" &
    [[ -n "${nid_col}" ]] && runnidhugg "${name%.*}" &
    [[ -n "${cds_col}" ]] && runcdschecker "${name%.*}" &
    [[ -n "${rcmc_col}" ]] && runrcmc "${name%.*}" &
}

runbenchmarks() {
    for dir in "${benchmarks[@]}"
    do
	name="${dir##*/}"
	if test "${plotmode}" == "y"
	then
	    runalltoolsfullypar "${dir}" "${name}"
	else
	    runalltools "${dir}" "${name}"
	fi

	if test "${tablemode}" == "y"
	then
	    printf "%-10s" "${name%.*}"
	    for i in $(seq 1 "${toolnum}")
	    do
		[[ "${tool[$i]}" == "herd" || "${tool[$i]}" == "cdschecker" ]] && printf " %-20s" "& ${tool_res[$i]}"
		[[ "${tool[$i]}" == "nidhugg" || "${tool[$i]}" == "rcmc" ]] && printf " %-50s" "& ${tool_res[$i]}"
	    done; echo '\\'
	fi
    done
}

runlitmus() {
    for dir in herd/*.litmus
    do
	name="${dir##*/}"
	if test -n "${fastrun}"
	then
	    case "${name}" in
		"big0.litmus"|"small0.litmus"|"small1.litmus"| \
		"ainc5.litmus"|"ainc6.litmus"|"ainc7.litmus"| \
		"binc4.litmus"|"casrot6.litmus"|"casrot8.litmus"| \
		"casw4.litmus"|"casw5.litmus") continue;;
		*)                                                     ;;
	    esac
	fi

	if test "${plotmode}" == "y"
	then
	    runalltoolsfullypar "${dir}" "${name}"
	else
	    runalltools "${dir}" "${name}"
	fi

	if test "${tablemode}" == "y"
	then
	    printf "%-10s" "${name%.*}"
	    for i in $(seq 1 "${toolnum}")
	    do
		[[ "${tool[$i]}" == "herd" || "${tool[$i]}" == "cdschecker" ]] && printf " %-20s" "& ${tool_res[$i]}"
		[[ "${tool[$i]}" == "nidhugg" || "${tool[$i]}" == "rcmc" ]] && printf " %-50s" "& ${tool_res[$i]}"
	    done; echo '\\'
	fi
    done
}


################################################################################
# COMMAND-LINE ARGUMENTS PARSING
################################################################################

# test whether arrays are supported
arrtest[0]='test' ||
    (echo 'Failure: arrays not supported in this version of bash.' && exit 2)

# test whether getopt works
getopt --test > /dev/null
if [[ $? -ne 4 ]]; then
    echo "`getopt --test` failed in this environment."
    exit 1
fi

# command-line arguments
SHORT=t,p,l,b,f,o:,i:
LONG=table,plot,litmus,bench,fast,herd:,cdschecker:,nidhugg:,sc,tso,pso,rcmc:,weakra,mo,wb,impl:,output:

# temporarily store output to be able to check for errors
PARSED=$(getopt --options $SHORT --longoptions $LONG --name "$0" -- "$@")
if [[ $? -ne 0 ]]; then
    # getopt has complained about wrong arguments to stdout
    exit 2
fi
# use eval with "$PARSED" to properly handle the quoting
eval set -- "$PARSED"

toolnum=0
# actually parse the options until we see --
while true; do
    case "$1" in
	-t|--table)
	    tablemode=y
	    shift
	    ;;
	-p|--plot)
	    plotmode=y
	    shift
	    ;;
	-l|--litmus)
	    litmusrun=y
	    shift
	    ;;
	--b|--bench)
	    benchrun=y
	    shift
	    ;;
	-f|--fast)
	    fastrun=y
	    shift
	    ;;
        --herd)
            [[ "$2" -gt 0 ]] && herd_col="$2" && tool["$2"]=herd
	    ((++toolnum))
            shift 2
            ;;
        --cdschecker)
            [[ "$2" -gt 0 ]] && cds_col="$2" && tool["$2"]=cdschecker
	    ((++toolnum))
            shift 2
            ;;
        --nidhugg)
            [[ "$2" -gt 0 ]] && nid_col="$2" && tool["$2"]=nidhugg
	    ((++toolnum))
            shift 2
            ;;
	--rcmc)
	    [[ "$2" -gt 0 ]] && rcmc_col="$2" && tool["${rcmc_col}"]=rcmc
	    ((++toolnum))
	    shift 2
	    ;;
	-i|--impl)
	    impl="$2"
	    shift 2
	    ;;
	--weakra)
	    run_weakra=weakra
	    shift
	    ;;
	--mo)
	    run_mo=mo
	    shift
	    ;;
	--wb)
	    run_wb=wb
	    shift
	    ;;
	--sc)
	    run_sc=sc
	    shift
	    ;;
	--tso)
	    run_tso=tso
	    shift
	    ;;
	--pso)
	    run_pso=pso
	    shift
	    ;;
        -o|--output)
            out_file="$2"
            shift 2
            ;;
        --)
            shift
            break
            ;;
        *)
            echo "Programming error"
            exit 3
            ;;
    esac
done

#default mode: tablemode
[[ "${plotmode}" == "y" ]] || tablemode="y"

#default implementation: C++
impl="${impl:-c++}"

# at least one model has to be specified
[[ -z "${run_mo}" ]] && [[ -z "${run_wb}" ]] && run_weakra="weakra"
[[ -z "${run_tso}" ]] && [[ -z "${run_pso}" ]] && run_sc="sc"

printtable() {
    # print a dummy header (LaTeX comment)
    printf "\n%% "; printline
    printf "%% order between weakra, mo, wb: weakra < mo < wb\n"
    printf "%% "; printline

    printf "\n%% ";printline
    printf "%% %-15s" "benchmark"
    for i in $(seq 1 "${toolnum}")
    do
	[[ "${tool[$i]}" == "herd" || "${tool[$i]}" == "cdschecker" ]] && printf " %-20s" "${tool[$i]}"
	[[ "${tool[$i]}" == "nidhugg" || "${tool[$i]}" == "rcmc" ]] && printf " %-40s" "${tool[$i]}"
    done;
    printf "\n%% ";printline

    # print actual table data according to command-line options
    if test -n "${benchrun}"
    then
	runbenchmarks
    else
	runlitmus
    fi
}

printplot() {
    # clean up old files
    for t in "${tool[@]}"
    do
	if test "${t}" == "herd" -o "${t}" == "cdschecker"
	then
	    > "${t}.out"
	elif test "${t}" == "nidhugg"
	then
	    for m in "${run_sc}" "${run_tso}" "${run_pso}"
	    do
		if test -z "$m"
		then
		    continue
		fi
		> "${t}.${m}.out"
	    done
	else
	    for m in "${run_weakra}" "${run_mo}" "${run_wb}"
	    do
		if test -z "$m"
		then
		    continue
		fi
		> "${t}.${m}.out"
	    done
	fi
    done

    # run benchmarks (in parallel) or litmus tests (sequentially)
    if test -n "${benchrun}"
    then
    	runbenchmarks
    else
    	runlitmus
    fi

    # wait for all child processes to finish...
    wait

    # sort results based on time for each tool and store the output
    for i in $(seq 1 "${toolnum}")
    do
	if test "${tool[$i]}" == "herd" -o "${tool[$i]}" == "cdschecker"
	then
	    cat "${tool[$i]}".out | awk '{ for (i = 1; i <= NF; i++) if ( $i != "&" && $i != "\\\\") printf "%-10s ", $i; print ""; }' | sort -nb -k2,2 > "${tool[$i]}".sorted
	elif test "${tool[$i]}" == "nidhugg"
	then
	    for m in "${run_sc}" "${run_tso}" "${run_pso}"
	    do
		if test -z "$m"
		then
		    continue
		fi
		cat "${tool[$i]}"."$m".out | awk '{ for (i = 1; i <= NF; i++) if ( $i != "&" && $i != "\\\\") printf "%-10s ", $i; print ""; }' | sort -nb -k2,2 > "${tool[$i]}"."$m".sorted
	    done
	else
	    for m in "${run_weakra}" "${run_mo}" "${run_wb}"
	    do
		if test -z "$m"
		then
		    continue
		fi
		cat "${tool[$i]}"."$m".out | awk '{ for (i = 1; i <= NF; i++) if ( $i != "&" && $i != "\\\\") printf "%-10s ", $i; print ""; }' | sort -nb -k2,2 > "${tool[$i]}"."$m".sorted
	    done
	fi
    done

    # plot the sorted outputs
    filenames=`ls *.sorted`
    tools="${tool[@]}"
    for file in *.sorted
    do
	gnuplot -e "filenames='${filenames}'" -e "tools='${tools}'" plot.plg
    done
}

if test "${plotmode}" == "y"
then
    printplot
else
    printtable
fi
