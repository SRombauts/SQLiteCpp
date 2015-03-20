@REM Copyright (c) 2012-2015 Sebastien Rombauts (sebastien.rombauts@gmail.com)
@REM
@REM Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
@REM or copy at http://opensource.org/licenses/MIT)
mkdir build
cd build

@REM Generate a Visual Studio solution for latest version found
cmake -DSQLITECPP_BUILD_EXAMPLES=ON -DSQLITECPP_BUILD_TESTS=ON ..

@REM Build default configuration (ie 'Debug')
cmake --build .

@REM prepare and launch tests
mkdir examples
mkdir examples\example1
copy ..\examples\example1\example.db3 examples\example1
copy ..\examples\example1\logo.png    examples\example1
ctest --output-on-failure
cd ..
