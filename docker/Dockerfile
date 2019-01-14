FROM ubuntu:xenial
MAINTAINER M Z


# Install dependencies
RUN apt-get update               \
 && apt-get -y -q upgrade        \
 && apt-get -y -q install        \
    bc                           \
    binutils-arm-linux-gnueabihf \
    build-essential              \
    ccache                       \
    gcc-arm-linux-gnueabihf      \
    libncurses-dev               \
    libssl-dev                   \
    u-boot-tools                 \
    wget                         \
    xz-utils                     \
    kmod                        \
 && apt-get clean



