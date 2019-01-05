#!/bin/bash

set -x

# config
CHEF_BUILD_TYPE=${CHEF_BUILD_TYPE:-release} # option: "release" "debug"

ROOT_DIR=$(pwd)

mkdir -p output && \
  cd output && \
  cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE $ROOT_DIR && \
  make
