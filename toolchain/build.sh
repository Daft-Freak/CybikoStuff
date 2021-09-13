#!/bin/sh
set -e

BINUTILS_VERSION=binutils-2.37
GCC_VERSION=gcc-8.5.0
NEWLIB_VERSION=newlib-4.1.0

JOBS=$(nproc --all)
PREFIX=$PWD/prefix
export PATH=$PATH:$PREFIX/bin

mkdir -p $PREFIX

NEWLIB_FLAGS="--disable-newlib-supplied-syscalls \
	--disable-nls \
	--enable-newlib-reent-small \
	--disable-newlib-fvwrite-in-streamio \
	--disable-newlib-fseek-optimization \
	--disable-newlib-wide-orient \
	--enable-newlib-nano-malloc \
	--disable-newlib-unbuf-stream-opt \
	--enable-lite-exit \
	--enable-newlib-global-atexit \
	--enable-newlib-nano-formatted-io"


mkdir -p build
cd build

# get sources
wget -nc https://ftpmirror.gnu.org/binutils/$BINUTILS_VERSION.tar.xz
tar --skip-old-files -xaf $BINUTILS_VERSION.tar.xz

wget -nc ftp://ftp.mirrorservice.org/sites/sourceware.org/pub/gcc/releases/$GCC_VERSION/$GCC_VERSION.tar.xz
tar --skip-old-files -xaf $GCC_VERSION.tar.xz

wget -nc ftp://sourceware.org/pub/newlib/$NEWLIB_VERSION.tar.gz
tar --skip-old-files -xaf $NEWLIB_VERSION.tar.gz

#binutils
cd $BINUTILS_VERSION
mkdir -p build
cd build
../configure --target=h8300-elf --prefix=$PREFIX
make -j $JOBS
make install

cd ../../

#gcc
cd $GCC_VERSION
mkdir -p build
cd build
# libstdc++ disabled due to internal compiler error
../configure --target=h8300-elf --prefix=$PREFIX --enable-languages=c,c++ --with-newlib --with-headers=../../$NEWLIB_VERSION/newlib/libc/include --disable-libssp --disable-libada --disable-libstdcxx --enable-initfini-array
make -j $JOBS
make install

cd ../../

# newlib
cd $NEWLIB_VERSION

mkdir -p build
cd build
../configure --target=h8300-elf --prefix=$PREFIX \
    $NEWLIB_FLAGS \
    CFLAGS_FOR_TARGET="-g -Os -ffunction-sections -fdata-sections"

make -j $JOBS
make install

cd ../../

