#!/bin/bash

# Get binary's full path
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
KATER="${KATER:-$DIR/../src/kater}"

echo 'Running all tests...'
for t in ${DIR}/../tests/*.kat
do
    echo -ne "${t##*/}: "
    if ! "${KATER}" "${t}" >/dev/null 2>&1
    then
    	echo "Error"
    	problem=1
    else
	echo "OK"
    fi
done

if [[ "${problem}" -eq 1 ]]
then
    echo '-----------------------------------'
    echo '--- UNEXPECTED TESTING RESULTS! ---'
    echo '-----------------------------------'
    exit 1
fi
echo '--- Testing proceeded as expected'
