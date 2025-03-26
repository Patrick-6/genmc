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

function floor {
    echo $(($1 / $2))
}

function ceil {
    echo $(( ($1+$2-1) / $2 ))
}

# if any of the files to be created already exists, bail
for ds in queue stack
do
    path="${DIR}/../tests/correct/relinche/${ds}"
    for model in "${MODELS[@]}"
    do
	for i in `seq 2 8`
	do
	    file="${path}/${ds}_spec_${model}_$(ceil $i 2)$(floor $i 2).in"
 	    if [[ -f "${file}" ]]
	    then
		echo "File $file already exists! Aborting."
		exit 1
	    fi
	done
    done
done

# generate all spec files
for ds in queue stack
do
    path="${DIR}/../tests/correct/relinche/${ds}"
    for model in "${MODELS[@]}"
    do
	for i in `seq 2 8`
	do
	    f="$(floor $i 2)"
	    c="$(ceil $i 2 )"
	    "${GenMC}" ${GENMCFLAGS} -collect-lin-spec="${path}/${ds}_spec_${model}_${c}${f}.in" -- -DWTN="${c}" -DRTN="${f}" -include "${path}/spec.c" "${path}/test_client.c"
	done
    done
    cat ${path}/${ds}_spec_${model}_*.in > "${path}/${ds}_spec_${model}.in"
    rm ${path}/${ds}_spec_${model}_*.in
done
