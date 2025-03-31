#!/bin/bash

# Driver script for running tests with GenMC.
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

# Get binary's full path
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
GenMC="${GenMC:-${DIR}/../genmc}"
GENMCFLAGS="${GENMCFLAGS:---disable-estimation --disable-mm-detector}"
MODELS=(rc11)
SPEC_ARGS=("-DSYNC_VAL" "-DSYNC_INS" "-DSYNC_REM" "-DSYNC_INS -DSYNC_REM")

declare -A spec_arg_print_map
spec_arg_print_map["-DSYNC_VAL"]="v"
spec_arg_print_map["-DSYNC_INS"]="vi"
spec_arg_print_map["-DSYNC_REM"]="vr"
spec_arg_print_map["-DSYNC_INS -DSYNC_REM"]="vir"

source "${DIR}/terminal.sh"

function floor {
    echo $(($1 / $2))
}

function ceil {
    echo $(( ($1+$2-1) / $2 ))
}


############################################################
## Parse CLI arguments
############################################################

while [[ $# -gt 0 ]]; do
    case "$1" in
	-p | --promote)
	    promote_mode=1
	    shift
	    ;;
	*)
	    shift
	    ;;
    esac
done


############################################################
## Run tests
############################################################

printline
echo '--- Running specification analysis...'
printline; echo ''

printline
printf "| ${CYAN}%-20s${NC} | ${CYAN}%-5s${NC} | ${CYAN}%-5s${NC} | ${CYAN}%-5s${NC} | ${CYAN}%-6s | ${CYAN}%-6s |\n" \
       "Data-structure" "Spec" "SR" "Model" "Result" "Time"
printline

total_time=0
for ds in queue stack
do
    path="${DIR}/../tests/correct/relinche/${ds}"
    for sarg in "${SPEC_ARGS[@]}"
    do
	for sr in "-disable-sr" ""
	do
	    [[ $sr = "" ]] && print_sr="YES" || print_sr="NO"
	    for model in "${MODELS[@]}"
	    do
		printf "| ${POWDER_BLUE}%-20s${NC} | ${POWDER_BLUE}%-5s${NC} | ${POWDER_BLUE}%-5s${NC} | ${POWDER_BLUE}%-5s${NC} | " \
		       "${ds}" "${spec_arg_print_map[$sarg]}" "${print_sr}" "${model}"
		test_time=0
		for i in `seq 1 4`
		do
		    for j in `seq 1 4`
		    do
			file="${path}/${ds}_spec_${spec_arg_print_map[$sarg]}_${model}_${i}${j}.in"
			output=$("${GenMC}" ${GENMCFLAGS} -collect-lin-spec="${file}" -- -DWTN="${i}" -DRTN="${j}" "${sarg}" -include "${path}/spec.c" "${path}/mpc.c" 2>&1)

			# increase time
			time=$(echo "${output}" | sed -n 's/.*Total wall-clock time: \([0-9\.]\+\).*/\1/p')
			test_time=$(echo "scale=2; ${test_time}+${time}" | bc -l)
		    done
		done

		# cat and delete tmp spec files
		file_prefix="${path}/${ds}_spec_${spec_arg_print_map[$sarg]}_${model}"
		cat_output=$(cat ${file_prefix}_*.in)
		rm ${file_prefix}_*.in

		# if in promote mode, save spec
		if [[ -n "${promote_mode}" ]]
		then
		    echo "${cat_output}" > "${file_prefix}.in"
		fi

		# check if output matches expected
		if ! diff "${file_prefix}.in" <(echo "${cat_output}")
		then
		    printf "%-6s | %-6s |\n" "ERROR" "${test_time}"
		    echo "${cat_output}"
		    failure=1
		else
		    printf "%-6s | %-6s |\n" "OK" "${test_time}"
		fi

		# update total time
		total_time=$(echo "scale=2; ${total_time}+${test_time}" | bc -l)
	    done
	done
    done
done

printline
echo '--- Total time: ' "${total_time}"
printline
