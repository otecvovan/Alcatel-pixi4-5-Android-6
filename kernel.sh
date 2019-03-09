#!/bin/bash

export ARCH=arm
export CROSS_COMPILE=~/arm-eabi-4.8-toolchain/bin/arm-eabi-
make O=out TARGET_ARCH=arm jhz6735m_35u_m_defconfig
make O=out TARGET_ARCH=arm | tee build.log
