# Copyright (c) 2012-2015 Sébastien Rombauts (sebastien.rombauts@gmail.com)
#
# Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
# or copy at http://opensource.org/licenses/MIT)
mkdir -p build
cd build

# Generate a Makefile for GCC (or Clang, depanding on CC/CXX envvar)
cmake -DSQLITECPP_BUILD_EXAMPLES=ON -DSQLITECPP_BUILD_TESTS=ON ..
# Build (ie 'make')
cmake --build .

# Prepare and run unit-tests (ie 'make test')
mkdir -p examples/example1
cp ../examples/example1/example.db3 examples/example1
cp ../examples/example1/logo.png    examples/example1
ctest --output-on-failure
