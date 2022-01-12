#!/bin/bash
#
# Rolls a new release to the public repository.
# It excludes internal and Gitlab-specific files such as:
#
#    - .gitlab-ci.yml
#    - gitlab-ci/
#    - Dockerfile
#    - .dockerignore
#    - /doc/manual.org
#    - clean-artifacts.sh
#    - roll-a-release.sh (the script itself)
#
# This script needs to be run *before* merging dev to master,
# otherwise the commit from which the patch should be created
# needs to be passed as an argument.

VERSION=0.7
INTERNAL_PATH=/home/michalis/Documents/genmc-tool
EXTERNAL_PATH=/home/michalis/Documents/genmc-github
PATCH_PATH=/tmp/release-"${VERSION}".patch
COMMIT=master

set -e

[ $# -gt 0 ] && COMMIT=$1 && shift

# update local github mirror
cd "${INTERNAL_PATH}"
git diff "${COMMIT}"..dev --binary -- . ':!./.gitlab-ci.yml' ':!./gitlab-ci' ':!./clean-artifacts.sh' ':!./export-github-patch.sh' ':!./Dockerfile' ':!./.dockerignore' ':!./roll-a-release.sh' ':!./doc/manual.org' > "${PATCH_PATH}"
git checkout master && git merge dev && git tag "v${VERSION}"
git push origin master && git push origin "v${VERSION}"

# update github
cd "${EXTERNAL_PATH}"
git checkout master && git apply "${PATCH_PATH}"
autoreconf --install && ./configure && make ftest
git add -A && git commit -s -m "Release GenMC v${VERSION}" && git tag "v${VERSION}"
git push origin master && git push origin "v${VERSION}"
git clean -dfX

# publish dockerfile
cd "${INTERNAL_PATH}"
sudo docker build --no-cache -t genmc:"v${VERSION}" .
sudo docker image tag genmc:"v${VERSION}" genmc/genmc:"v${VERSION}"
sudo docker image tag genmc:"v${VERSION}" genmc/genmc:latest
sudo docker image push genmc/genmc:"v${VERSION}"
sudo docker image push genmc/genmc:latest

echo "A new release has been rolled."
