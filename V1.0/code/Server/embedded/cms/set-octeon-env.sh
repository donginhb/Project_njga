#!/bin/bash

# OCTEON SDK environment variables
# install directory
export OCTEON_ROOT=/usr/local/Cavium_Networks/OCTEON-SDK

pushd $OCTEON_ROOT
source env-setup OCTEON_CN61XX
popd

