FROM debian:bullseye
LABEL maintainer="michalis@mpi-sws.org"

WORKDIR /root

RUN export TERM='xterm-256color'

# fetch all necessary packages
RUN apt-get update
RUN apt-get install -y -qq wget gnupg gnupg2 libtinfo5 bc \
    autoconf make automake libffi-dev zlib1g-dev libedit-dev \
    libxml2-dev xz-utils g++ clang git util-linux
RUN apt-get install -y -qq \
        clang-9 libclang-9-dev llvm-9 llvm-9-dev

# clone and build
RUN git clone -v https://github.com/MPI-SWS/genmc.git
RUN cd genmc && autoreconf --install && ./configure --with-llvm=/usr/lib/llvm-9 && make && make install
