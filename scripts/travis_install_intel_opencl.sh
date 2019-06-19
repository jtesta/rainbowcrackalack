#!/bin/bash

wget https://github.com/intel/compute-runtime/releases/download/19.23.13131/intel-gmmlib_19.2.1_amd64.deb
wget https://github.com/intel/compute-runtime/releases/download/19.23.13131/intel-igc-core_1.0.8-2084_amd64.deb
wget https://github.com/intel/compute-runtime/releases/download/19.23.13131/intel-igc-opencl_1.0.8-2084_amd64.deb
wget https://github.com/intel/compute-runtime/releases/download/19.23.13131/intel-opencl_19.23.13131_amd64.deb
wget https://github.com/intel/compute-runtime/releases/download/19.23.13131/intel-ocloc_19.23.13131_amd64.deb

dpkg -i *.deb
