This example demonstrates how to use SQLiteCpp *within the implementation* of a
[SQLite3 loadable extension](https://sqlite.org/loadext.html). Change into this directory and

```
cmake -B build .
cmake --build build
build/example_driver $(pwd)/build/libexample.so
```

*(replace .so with .dylib or .dll if appropriate)*

This should print `it works 42`. Here the `example_driver` program links SQLite3 *statically*, so
it's important to ensure that SQLiteCpp inside the extension will use that "copy" of SQLite3 rather
than trying to dynamically link another one. See [CMakeLists.txt](CMakeLists.txt) for the key CMake
option that ensures this, and [src/example_extension.cpp](src/example_extension.cpp) for some
necessary boilerplate.
