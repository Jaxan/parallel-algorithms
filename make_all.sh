#!/bin/bash

set -o nounset #Error when unset vars are used
set -o errexit #Exit on errors
set -o pipefail #Causes failure when input-pipe-program fails
set -x

mkdir -p build-$1
cd build-$1
cmake -DCMAKE_BUILD_TYPE=$1 ..
make -j4
cd ..
