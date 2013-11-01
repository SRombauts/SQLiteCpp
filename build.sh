# Copyright (c) 2013 Sébastien Rombauts (sebastien.rombauts@gmail.com)
# 
# Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
# or copy at http://opensource.org/licenses/MIT)

mkdir -p build
cd build
# generate solution for GCC
cmake ..
cmake --build .

# prepare and launch tests
mkdir -p examples/example1
cp ../examples/example1/example.db3 examples/example1
cp ../examples/example1/logo.png    examples/example1
ctest .
