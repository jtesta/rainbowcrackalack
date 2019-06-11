#!/bin/bash

make -f Makefile.aws check TARGETS=hw DEVICES=$AWS_PLATFORM all | tee ~/fpga_kernel_compile.txt
