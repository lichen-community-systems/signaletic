FROM jarnoharno/emsdk:latest

RUN apt update && apt install -y python3 python3-pip \
python3-setuptools python3-wheel
RUN pip3 install meson

WORKDIR /gcc-arm
ARG TARGETARCH
RUN if [ "$TARGETARCH" = "amd64" ]; \
    then export TOOLCHAIN_ARCH="x86_64"; \
    else export TOOLCHAIN_ARCH="aarch64"; \
    fi; \
    wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/10-2020q4/gcc-arm-none-eabi-10-2020-q4-major-$TOOLCHAIN_ARCH-linux.tar.bz2; \
    tar -xf gcc-arm-none-eabi-10-2020-q4-major-$TOOLCHAIN_ARCH-linux.tar.bz2;

ENV GCC_PATH=/gcc-arm/gcc-arm-none-eabi-10-2020-q4-major/bin
RUN export PATH=$GCC_PATH:$PATH
RUN echo $GCC_PATH

WORKDIR /signaletic
