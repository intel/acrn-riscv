#!/bin/bash
set -x

make clean ARCH=riscv
make -j1 ARCH=riscv
