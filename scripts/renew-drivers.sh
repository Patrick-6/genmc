#!/bin/bash

# Driver script for running tests with GenMC.
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

# Get binary's full path
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
KATER="${KATER:-$DIR/../kater-tool/RelWithDebInfo/kater}"

# Ensure kater exists
if ! command -v "${KATER}" -h &> /dev/null
then
    echo "kater was not found"
    exit 1
fi

# used variables
IN_DIR="${IN_DIR:-${KATER%/*}/../kat}"
OUT_DIR="${OUT_DIR:-$DIR/../src/ExecutionGraph/Consistency}"
DRIVERS="${DRIVERS:-rc11 imm sc tso ra}"

for driver in ${DRIVERS} # break lines
do
    $("${KATER}" -e -p"${OUT_DIR}" -n"${driver}" "${IN_DIR}/${driver}-genmc.kat")
    echo "exporting " $driver $?
done
