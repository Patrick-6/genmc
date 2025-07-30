#!/bin/bash

# Randomizes test running
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
source "${DIR}/terminal.sh"

iterations="${iterations:-10}"

for i in `seq 1 "${iterations}"`
do
    echo -ne "Iteration: $i"
    res=$(GENMCFLAGS="${GENMCFLAGS} -schedule-policy=arbitrary -print-schedule-seed" "${DIR}/driver.sh" --fast --debug)
    if [[ $? -ne 0 ]]
    then
	echo "${RED} Error detected! ${NC}"
	echo "$res"
	exit 1
    fi
    echo -ne "\r"
    sleep .$[ ( $RANDOM % 20 ) + 1 ]s
done
