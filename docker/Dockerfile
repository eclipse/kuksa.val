FROM ubuntu:disco

LABEL maintainer="Bojan Miladinovic <bojan.kv.ns@gmail.com>"

RUN apt-get -qq update \
    && apt-get -qq install -y --no-install-recommends \
       sudo=1.* \
       wget=1.* \
       git=1:2.* \
       libc6-dev=2.* \
       g++-9-multilib \
       clang \
       clang-tools \
       llvm \
       nasm=2.* \
       libssl-dev=1.* \
       pkg-config=0.* \
       zlib1g-dev=1:1.* \
       libbz2-dev=1.* \
       curl=7.* \
       liblzma-dev=5.* \
       libboost1.67-all-dev \
       make=4.* \
       libglib2.0-dev \
       net-tools \
       python3-pip \
       vim \
    && pip3 install setuptools \
    && pip3 install gcovr \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 100 \
    && update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-9 100 \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 100 \
    && update-alternatives --install /usr/bin/cc cc /usr/bin/gcc-9 100 \
    && update-alternatives --install /usr/bin/gcov gcov /usr/bin/gcov-9 100 \
    && ln -s /usr/include/locale.h /usr/include/xlocale.h \
    && rm -rf /var/lib/apt/lists/* \
    && wget --no-check-certificate --quiet https://cmake.org/files/v3.15/cmake-3.15.3-Linux-x86_64.tar.gz \
    && tar -xzf cmake-3.15.3-Linux-x86_64.tar.gz \
       --exclude=bin/cmake-gui \
       --exclude=doc/cmake \
       --exclude=share/cmake-3.12/Help \
    && cp -fR cmake-3.15.3-Linux-x86_64/* /usr \
    && rm -rf cmake-3.15.3-Linux-x86_64 \
    && rm cmake-3.15.3-Linux-x86_64.tar.gz

WORKDIR /home

COPY build-gcc-default.sh /home
COPY build-gcc-coverage.sh /home

COPY build-clang-default.sh /home
COPY build-clang-coverage.sh /home

EXPOSE 8090
