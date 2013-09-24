# Copyright (c) 2013 Sébastien Rombauts (sebastien.rombauts@gmail.com)
# 
# Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
# or copy at http://opensource.org/licenses/MIT)

mkdir -p build
cd build
# generate solution for GCC
cmake -DSQLITECPP_RUN_CPPLINT=ON -DSQLITECPP_RUN_CPPCHECK=ON -DSQLITECPP_RUN_DOXYGEN=ON -DSQLITECPP_BUILD_EXAMPLES=ON -DSQLITECPP_BUILD_TESTS=ON ..
cmake --build .

# prepare and launch tests
mkdir -p examples/example1
cp ../examples/example1/example.db3 examples/example1
cp ../examples/example1/logo.png    examples/example1
ctest --output-on-failure
