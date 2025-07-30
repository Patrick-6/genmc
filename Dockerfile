FROM debian:trixie
LABEL maintainer="michalis@mpi-sws.org"

WORKDIR /root

RUN export TERM='xterm-256color'

# FIXME(cmake): Update dependencies
# fetch all necessary packages
RUN apt-get update
RUN apt-get install -y -qq wget gnupg gnupg2 bc \
    make cmake libffi-dev zlib1g-dev libedit-dev \
    libxml2-dev xz-utils g++ clang git util-linux
RUN apt-get install -y -qq clang-18 llvm-18 llvm-18-dev

# clone and build
RUN git clone -v https://github.com/MPI-SWS/genmc.git
RUN cd genmc && cmake -DCMAKE_PREFIX_PATH=/usr/lib/llvm-18 -DBUILD_DOC=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo -B RelWithDebInfo -S . && cmake --build RelWithDebInfo -j8 && cmake --install RelWithDebInfo
