#!/bin/bash

# Get binary's full path
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
KATER="${KATER:-$DIR/../src/kater}"

declare -A tests
tests=(["sc"]=1 ["tso"]=1)

for t in "${!tests[@]}"
do
    if ! diff <(cat "${DIR}/expected/${t}.out")  <("${KATER}" -v1 "cat/${t}.cat" 2>&1)
    then
	echo "Error: ${t}.cat"
	problem=1
    fi
done

if [[ "${problem}" -eq 1 ]]
then
    echo '--- UNEXPECTED TESTING RESULTS!'
    exit 1
fi
echo '--- Testing proceeded as expected'
