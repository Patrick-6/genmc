#!/bin/bash

# Test driver for testing ReLinChe
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
# Author: Pavel Golovin <gopavel0@gmail.com>

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
GenMC="${GenMC:-$DIR/../genmc}"

source "${DIR}/terminal.sh"

model="${model:-rc11}"
coherence="${coherence:-mo}"
testdir="${testdir:-${DIR}/../tests/correct/relinche}"

runtime=0
tests_success=0
tests_fail=0
result=0

shopt -s nullglob

print_spec_header() {
	# Print status
	echo ''
	printline
	echo -n '--- Preparing to analyze specs in '
	echo "${testdir##*/}" 'under' "${model}" 'with' "${coherence}" |
		awk '{ print toupper($1), $2, toupper($3), $4, toupper($5) }'
	printline
	echo ''

	# Print table's header
	printline
	printf "| ${CYAN}%-29s${NC} | ${CYAN}%-4s${NC} | ${CYAN}%-6s${NC} | ${CYAN}%-6s${NC} | ${CYAN}%-6s${NC} |\n" \
	       "Testcase" "Spec" "Result" "Spec execs" "Spec hints"
	printline
}

print_impl_header() {
	# Print status
	echo ''
	printline
	echo -n '--- Preparing to check conformance in '
	echo "${testdir##*/}" 'under' "${model}" 'with' "${coherence}" |
		awk '{ print toupper($1), $2, toupper($3), $4, toupper($5) }'
	printline
	echo ''

	# Print table's header
	printline
	printf "| ${CYAN}%-29s${NC} | ${CYAN}%-4s${NC} | ${CYAN}%-6s${NC} | ${CYAN}%-6s${NC} | ${CYAN}%-6s${NC} |\n" \
	       "Testcase" "Spec" "Result" "Impl execs" "Hints checked"
	printline
}

printfooter() {
    printline
    echo '--- Test time: ' "${runtime}"
    printline
    echo ''
}

print_spec_info() {
	line_suffix=""
	if [ "${test_line_num}" -gt 1 ]; then
		line_suffix=" (${test_line_num})"
	fi
	[[ -z "$sr" ]] && sr_suffix="SR" || sr_suffix="NOSR"
	SPEC_TYPE="V"
	if [[ "${args_in}" =~ "DSYNC_INS" ]]; then SPEC_TYPE="${SPEC_TYPE}I"; fi
	if [[ "${args_in}" =~ "DSYNC_REM" ]]; then SPEC_TYPE="${SPEC_TYPE}R"; fi
	printf "| ${POWDER_BLUE}%-29s${NC} | ${POWDER_BLUE}%-4s${NC} | " "${test_name:5:-3}${line_suffix}/${sr_suffix}" "${SPEC_TYPE}"
}

print_spec_results() {
    if test -n "${outcome_failure}"; then
	outcome="${RED}ERROR ${NC}"
	result=1
    elif test -n "${failure}"; then
	outcome="${LIME_YELLOW}FAILED${NC}"
	result=1
    else
	outcome="${GREEN}SAFE  ${NC}"
    fi
    printf "${outcome} | % 10s | % 10s |\n" \
	   "${col_execs}"  "${col_hints}"

    if test -n "${failure}" -o -n "${outcome_failure}"; then
	echo "${failure_output}"
    fi
}

print_impl_results() {
    if test -n "${outcome_failure}"; then
	outcome="${RED}ERROR ${NC}"
	result=1
    elif test -n "${failure}"; then
	outcome="${LIME_YELLOW}FAILED${NC}"
	result=1
    else
	outcome="${GREEN}SAFE  ${NC}"
    fi
    printf "${outcome} | % 10s | % 10s |\n" \
	   "${che_execs}"  "${che_hints}"

    if test -n "${failure}" -o -n "${outcome_failure}"; then
	echo "${failure_output}"
    fi
}

check_client_extension() {
    che_output_s=$(echo "${che_output}" | tr '\n' ' ')
    if ! [[ "${che_output_s}" =~ "The library implementation returns incorrect return values" ]]
    then
	return;
    fi;

    col_args=$(echo "${args_in}" | sed "s@-include [^ ]*@-include ${spec}@")
    che_args=$(echo "${args_in}")
    extension_flags=$(echo "${che_output_s}" | grep 'DGENERATE_SYNC' | sed 's/.*\(-DGENERATE_SYNC \(-D[^ ]* \)*\).*/\1/')
    col_ext_cmd="${GenMC} ${GENMCFLAGS} ${COL_FLAGS} --collect-lin-spec=${spec_file} --max-hint-size=0 -- ${extension_flags} ${col_args} ${client} 2>&1"
    if test -n "${print_cmd}"; then printf "\n${col_ext_cmd}\n"; fi # debug print
    col_ext_output=$(eval "${col_ext_cmd}")
    if test "$?" -ne 0; then
	failure_output="Specification analysis failed for extended client"
	failure=1
    fi
    che_ext_cmd="${GenMC} ${GENMCFLAGS} ${CHE_FLAGS} --check-lin-spec=${spec_file} --max-hint-size=0 -- ${extension_flags} ${che_args} ${client} 2>&1"
    if test -n "${print_cmd}"; then printf "\n${che_ext_cmd}\n"; fi # debug print
    che_ext_output=$(eval "${che_ext_cmd}")
    if test "$?" -eq 0; then
	failure_output="Erroneous extension is found, but extend client passed!"
	failure=1
    fi
}

# reads: col_hints, che_hints
# writes: failure_output, failure
check_spec_hints() {
    hint_num="${col_hints}"
    if test -n "${promote_mode}"; then
	echo -ne "${hint_num}" >> "${expected_file}" # impl will add newline
    else
	expected_results=$(sed "${test_line_num}q;d" "${expected_file}" | cut -d ' ' -f1)
	if test "${hint_num}" != "${expected_results}"; then
	    failure_output="Expected number of collected hints: ${expected_results}; actual: ${hint_num}"
	    failure=1
	fi
    fi
}

check_impl_hints() {
    hint_num="${che_hints}"
    if test -n "${promote_mode}"; then
	echo " ${hint_num}" >> "${expected_file}"
    else
	expected_results=$(sed "${test_line_num}q;d" "${expected_file}" | cut -d ' ' -f2)
	if test "${hint_num}" != "${expected_results}"; then
	    failure_output="Expected number of checked hints: ${expected_results}; actual: ${hint_num}"
	    failure=1
	fi
    fi
}

# reads: che_output_s
# writes: failure_output, failure
check_error_message() {
    error_message_s=$(echo "${che_output_s}" | sed -n 's/.*\(The library implementation.*\)\ \ \ .*/\1/p')
    expected_results=$(sed "${test_line_num}q;d" "${expected_file}")
    if test -n "${promote_mode}"; then
	echo "${error_message_s}" >> "${expected_file}"
    elif test "${error_message_s}" != "${expected_results}"; then
	NL=$'\n'
	failure_output="Expected error message: ${expected_results}${NL}Actual error message:   ${error_message_s}"
	failure=1
    fi
}

[ -z "${TESTFILTER}" ] && TESTFILTER=*

# Read files test.*.in in the folder $1 and interpret each line of such file
# as arguments for client '$1/test_client.c'. Specification uses the same arguments
# but replaces the `-include PATH' parameter with '-include $1/spec.c'
runspec() {
    failure=""
    failure_output=""
    dir="$1"
    spec="${dir}/spec.c"
    client="${dir}/mpc.c"
    GENMCFLAGS="${GENMCFLAGS:--disable-estimation --disable-mm-detector --rc11}"
    for test_in in "${dir}"/args.*.in; do
	for sr in "" "-disable-sr"; do
	    # skip big tests
	    if test -n "${fastrun}"; then
		if [[ "${test_in}" =~ "NxN" ]]; then
		    continue
		fi
	    fi

	    # filter tests
	    case "${test_in}" in
		${TESTFILTER}) ;;
		*) continue;;
	    esac

	    # check whether test is negative
	    should_fail=""
	    if [[ "${test_in}" =~ "fail" ]]; then
		should_fail=1
	    fi
	    test_line_num=0

	    # find expected file and recreate it in --promote mode
	    expected_file=$(echo "${test_in}" | sed 's/args\./expected./')
	    if test -n "${promote_mode}"; then
		[ -e "${expected_file}" ] && rm -v "${expected_file}";
		touch "${expected_file}"
	    fi

	    # run all subtest in the file
	    while read args_in || [[ $args_in ]]; do
		((test_line_num++))
		failure=""
		outcome_failure=""
		failure_output=""
		test_name="$(basename ${test_in})"
		spec_file="${dir}/${test_name%.in}-${test_line_num}.spec"

		print_spec_info

		# specification analysis
		col_args=$(echo "${args_in}" | sed "s@-include [^ ]*@-include ${spec}@")
		col_cmd="${GenMC} ${GENMCFLAGS} ${sr} --collect-lin-spec=${spec_file} ${COL_FLAGS} -- ${col_args} ${client} 2>&1"
		if test -n "${print_cmd}"; then printf "\n${col_cmd}\n"; fi # debug print
		col_output=$(eval "${col_cmd}")
		ret_code="$?"
		if test "$ret_code" -ne 0; then
		    failure_output=${col_output}
		    failure=1
		    print_spec_results
		    continue
		fi

		# result extraction
		col_output_s=$(echo "${col_output}" | tr '\n' ' ')
		col_execs=$(echo "${col_output_s}" | sed -n 's/.*Number of complete executions explored: \([0-9]\+\).*/\1/p')
		col_hints=$(echo "${col_output_s}" | sed -n 's/.*Number of hints found: \([0-9]\+\).*/\1/p')
		col_time=$(echo "${col_output_s}" | sed -n 's/.*Total wall-clock time: \([0-9\.]\+\).*/\1/p')
		# col_hint_time=$(echo "${col_output_s}" | sed -n 's/.*Relinche: Hint calculation complete, wall-clock time: \([0-9\.]\+\)s.*/\1/p')
		# col_genmc_time=$(echo "${col_time}-${col_hint_time}" | bc -l)

		if test "$ret_code" -ne 0; then
		    check_spec_hints
		fi

		# update time counter
		runtime=$(echo "scale=2; ${runtime}+${col_time}" | bc -l)
		print_spec_results
	    done <"${test_in}"
	done
    done
}

runimpl() {
    failure=""
    failure_output=""
    dir="$1"
    spec="${dir}/spec.c"
    client="${dir}/mpc.c"
    GENMCFLAGS="${GENMCFLAGS:--disable-estimation --disable-mm-detector --rc11}"
    che_hint_time="" # requires genmc to be compiled with -DGENMC_DEBUG
    che_genmc_time=""
    for test_in in "${dir}"/args.*.in; do
	# skip big tests
	if test -n "${fastrun}"; then
	    if [[ "${test_in}" =~ "NxN" ]]; then
		continue
	    fi
	fi

	# filter tests
	case "${test_in}" in
	    ${TESTFILTER}) ;;
	    *) continue;;
	esac

	# check whether test is negative
	should_fail=""
	if [[ "${test_in}" =~ "fail" ]]; then
	    should_fail=1
	fi

	test_line_num=0
	# find expected file and recreate it in --promote mode
	expected_file=$(echo "${test_in}" | sed 's/args\./expected./')
	if test -n "${promote_mode}"; then
	    [ -e "${expected_file}" ] && rm -v "${expected_file}";
	    touch "${expected_file}"
	fi

	# run all subtest in the file
	while read args_in || [[ $args_in ]]; do
	    ((test_line_num++))
	    failure=""
	    outcome_failure=""
	    failure_output=""
	    test_name="$(basename ${test_in})"
	    spec_file="${dir}/${test_name%.in}-${test_line_num}.spec"

	    print_spec_info

	    che_args=$(echo "${args_in}")

	    # check conformance
	    che_cmd="${GenMC} ${GENMCFLAGS} --check-lin-spec=${spec_file} ${CHE_FLAGS} -- ${che_args} ${client} 2>&1"
	    if test -n "${print_cmd}"; then printf "\n${che_cmd}\n"; fi # debug print
	    che_output=$(eval "${che_cmd}")
	    ret_code="$?"
	    che_output_s=$(echo "${che_output}" | tr '\n' ' ')
	    if [ "${ret_code}" -ne 0 ] && [ -z "${should_fail}" ]; then # failed but shouldn't
		failure_output=${che_output}
		failure=1
		rm "${spec_file}"
		print_impl_results
		continue
	    fi
	    if [ "${ret_code}" -ne 0 ] && [ -n "${should_fail}" ]; then # failed as expected
		check_error_message
		if test -z "${failure}"; then
		    check_client_extension
		fi
		rm "${spec_file}"
		print_impl_results
		continue
	    fi
	    if [ "${ret_code}" -eq 0 ] && [ -n "${should_fail}" ]; then # didn't fail but should
		failure_output="Linearizability error in not detected in negative test"
		failure=1
		rm "${spec_file}"
		print_impl_results
		continue
	    fi

	    # extract results from the checking phase output
	    che_execs=$(echo "${che_output_s}" | sed -n 's/.*Number of complete executions explored: \([0-9]\+\).*/\1/p')
	    che_hints=$(echo "${che_output_s}" | sed -n 's/.*Number of checked hints: \([0-9]\+\).*/\1/p')
	    che_time=$(echo "${che_output_s}" | sed -n 's/.*Total wall-clock time: \([0-9\.]\+\).*/\1/p')
	    che_hint_time=$(echo "${che_output_s}" | sed -n 's/.*Relinche time: \([0-9\.]\+\)s.*/\1/p')
	    if test -n "${che_hint_time}"; then
		che_genmc_time=$(echo "${che_time}-${che_hint_time}" | bc -l)
	    fi
	    check_impl_hints

	    # update total time counter
	    runtime=$(echo "scale=2; ${runtime}+${che_time}" | bc -l)
	    rm "${spec_file}"
	    print_impl_results
	done <"${test_in}"
    done
}


# Parse arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
	-f | --fast)
	    fastrun=1
	    shift
	    ;;
	-c | --print-cmd)
	    print_cmd=1
	    shift
	    ;;
	-p | --promote)
	    promote_mode=1
	    shift
	    ;;
	*)
	    shift
	    ;;
    esac
done

# Run correct testcases and update status
print_spec_header
for dir in "${testdir}"/*/; do
    runspec "${dir}"
done
printfooter

runtime=0

print_impl_header
for dir in "${testdir}"/*/; do
    runimpl "${dir}"
done
printfooter

exit "${result}"
