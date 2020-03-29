#!/bin/bash
#
# Exports a series of patches to be applied to the Github repository.
# It excludes internal and Gitlab-specific files such as:
#
#    - .gitlab-ci.yml
#    - /doc/manual.org
#    - clean-artifacts.sh
#    - export-github-patch.sh (the script itself)
#
# This script should be run from the dev branch right before merging
# to master, otherwise the commit from which the patch should be
# created needs to be passed as an argument.

COMMIT=master

[ $# -gt 0 ] && COMMIT=$1 && shift

git diff "${COMMIT}" --binary -- . ':!./.gitlab-ci.yml' ':!./clean-artifacts.sh' ':!./export-github-patch.sh' ':!./doc/manual.org'
