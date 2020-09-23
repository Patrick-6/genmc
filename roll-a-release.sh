#!/bin/bash
#
# Rolls a new release to the public repository.
# It excludes internal and Gitlab-specific files such as:
#
#    - .gitlab-ci.yml
#    - gitlab-ci/
#    - /doc/manual.org
#    - clean-artifacts.sh
#    - roll-a-release.sh (the script itself)
#
# This script needs to be run *before* merging dev to master,
# to master, otherwise the commit from which the patch should be
# created needs to be passed as an argument.

VERSION=0.5
INTERNAL_PATH=/home/michalis/Documents/genmc-tool
EXTERNAL_PATH=/home/michalis/Documents/genmc-github
PATCH_PATH=/tmp/release-"${VERSION}".patch
COMMIT=master

[ $# -gt 0 ] && COMMIT=$1 && shift

cd "${INTERNAL_PATH}" &&
git diff "${COMMIT}"..dev --binary -- . ':!./.gitlab-ci.yml' ':!./gitlab-ci' ':!./clean-artifacts.sh' ':!./export-github-patch.sh' ':!./roll-a-release.sh' ':!./doc/manual.org' > "${PATCH_PATH}" &&
git checkout master && git merge dev && git tag "v${VERSION}" &&
git push origin master && git push origin "v${VERSION}" &&

cd "${EXTERNAL_PATH}" &&
git checkout master && git apply "${PATCH_PATH}" &&
autoreconf --install && ./configure && make ftest &&
git add -A && git commit -s -m "Release GenMC v${VERSION}" && git tag "v${VERSION}" &&
git push origin master && git push origin "v${VERSION}" &&
git clean -dfX &&

echo "A new release has been rolled."
