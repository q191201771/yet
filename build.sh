#!/bin/bash

set -x

# config
CHEF_BUILD_TYPE=${CHEF_BUILD_TYPE:-Release} # option: "Release" "Debug"

ROOT_DIR=$(pwd)

mkdir -p output && \
  cd output && \
  cmake -DCMAKE_BUILD_TYPE=$CHEF_BUILD_TYPE $ROOT_DIR && \
  make
