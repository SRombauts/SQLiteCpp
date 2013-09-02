@REM Copyright (c) 2013 Sébastien Rombauts (sebastien.rombauts@gmail.com)
@REM 
@REM Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
@REM or copy at http://opensource.org/licenses/MIT)

mkdir build
cd build
@REM generate solution for Visual Studio 2010, and build it
cmake .. -G "Visual Studio 10"
cmake --build .

@REM prepare and launch tests
mkdir examples
mkdir examples\example1
copy ..\examples\example1\example.db3 examples\example1
copy ..\examples\example1\logo.png    examples\example1
ctest .
