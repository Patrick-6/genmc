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
# This script needs to be run *before* merging dev to master.

VERSION=0.5
INTERNAL_PATH=/home/michalis/Documents/genmc-tool
EXTERNAL_PATH=/home/michalis/Documents/genmc-github
PATCH_PATH=/tmp/release-"${VERSION}".patch

cd "${INTERNAL_PATH}" &&
git diff master..dev --binary -- . ':!./.gitlab-ci.yml' ':!./gitlab-ci' ':!./clean-artifacts.sh' ':!./roll-a-release.sh' ':!./doc/manual.org' > "${PATCH_PATH}" &&
git checkout master && git merge dev && git tag "v${VERSION}" && git push origin master &&

cd "${EXTERNAL_PATH}" &&
git checkout master && git apply "${PATCH_PATH}" &&
autoreconf --install && ./configure && make ftest &&
git add -A && git commit -s -m "Release GenMC v${VERSION}" && git tag "v${VERSION}" &&
git push origin master && git clean -dfX &&

echo "A new release has been rolled."
