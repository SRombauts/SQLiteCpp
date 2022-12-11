project(
    'SQLiteCpp', 'cpp',
    # SQLiteCpp requires C++11 support
    default_options: ['cpp_std=c++11'],
    license: 'MIT',
    version: '3.2.0',
)

cxx = meson.get_compiler('cpp')

## at best we might try to test if this code compiles
## testing for compilers or platforms is not reliable enough
## example: native clang on windows or mingw in windows
unix_like_code = '''
    #if defined(unix) || defined(__unix__) || defined(__unix)
    // do nothing
    #else
    # error "Non Unix-like OS"
    #endif
'''
unix_like = cxx.compiles(unix_like_code, name : 'unix like environment')

## C++17 disable the support for std::filesystem (by default off)
if get_option('SQLITECPP_DISABLE_STD_FILESYSTEM')
    sqlitecpp_cxx_flags += ['-DSQLITECPP_DISABLE_STD_FILESYSTEM']
endif

thread_dep = dependency('threads')
# sqlite3 support
sqlite3_dep = dependency(
                'sqlite3',
                fallback: ['sqlite3', 'sqlite3_dep']
)

sqlitecpp_incl = [ 
    include_directories('include')
]
sqlitecpp_srcs = [
    'src/Backup.cpp',
    'src/Column.cpp',
    'src/Database.cpp',
    'src/Exception.cpp',
    'src/Savepoint.cpp',
    'src/Statement.cpp',
    'src/Transaction.cpp',
]
sqlitecpp_args = [
    '-Wall',
]
sqlitecpp_link = []
sqlitecpp_deps = [
    sqlite3_dep,
    thread_dep,
]
## used to override the default sqlitecpp options like cpp standard
sqlitecpp_opts = []

## tests

sqlitecpp_test_srcs = [
    'tests/Column_test.cpp',
    'tests/Database_test.cpp',
    'tests/Savepoint_test.cpp',
    'tests/Statement_test.cpp',
    'tests/Backup_test.cpp',
    'tests/Transaction_test.cpp',
    'tests/VariadicBind_test.cpp',
    'tests/Exception_test.cpp',
    'tests/ExecuteMany_test.cpp',
]
sqlitecpp_test_args = []

## samples

sqlitecpp_sample1_srcs = [
    'examples/example1/main.cpp',
]
sqlitecpp_sample2_srcs = [
    'examples/example2/src/main.cpp',
]

# if not using MSVC we need to add this compiler arguments
# for a list of MSVC supported arguments please check:
# https://docs.microsoft.com/en-us/cpp/build/reference/compiler-options-listed-alphabetically?view=msvc-170
if not (host_machine.system() == 'windows' and cxx.get_id() == 'msvc')
    sqlitecpp_args += [
        '-Wextra',
        '-Wpedantic',
        '-Wswitch-enum',
        '-Wshadow',
        '-Wno-long-long',
    ]
endif
## using MSVC headers requires c++14, if not will show an error on xstddef as: 
## 'auto' return without trailing return type; deduced return types are a C++14 extension
if host_machine.system() == 'windows'
    message('[WINDOWS] using c++14 standard')
    sqlitecpp_opts += [
        'cpp_std=c++14',
    ]
    # check that we are not trying to build as dynamic library
    if get_option('default_library') != 'shared'
        warming('SQLiteCpp does not support shared library on Windows, the library will be built as static')
    endif
    
endif
# Options relative to SQLite and SQLiteC++ functions

if get_option('SQLITE_ENABLE_COLUMN_METADATA')
    sqlitecpp_args += [
        '-DSQLITE_ENABLE_COLUMN_METADATA',
    ]
endif

if get_option('SQLITE_ENABLE_ASSERT_HANDLER')
    sqlitecpp_args += [
        '-DSQLITE_ENABLE_ASSERT_HANDLER',
    ]
endif

if get_option('SQLITE_HAS_CODEC')
    sqlitecpp_args += [
        'SQLITE_HAS_CODEC',
    ]
endif

if get_option('SQLITE_USE_LEGACY_STRUCT')
    sqlitecpp_args += [
        '-DSQLITE_USE_LEGACY_STRUCT',
    ] 
endif

if unix_like
    sqlitecpp_args += [
        '-DfPIC',
    ]
    # add dl dependency
    libdl_dep = cxx.find_library('dl')
    sqlitecpp_deps += [
        libdl_dep,
    ]
endif

if get_option('b_coverage')
    # Prevent the compiler from removing the unused inline functions so that they get tracked as "non-covered"
    sqlitecpp_args += [
        '-fkeep-inline-functions',
        '-fkeep-static-functions',
    ]
endif

## Workarround for windows: if building on windows we will build the library as static
if host_machine.system() == 'windows'
    libsqlitecpp = static_library(
        'sqlitecpp',
        sqlitecpp_srcs,
        include_directories: sqlitecpp_incl,
        cpp_args: sqlitecpp_args,
        dependencies: sqlitecpp_deps,
        # override the default options
        override_options: sqlitecpp_opts,)
else
    libsqlitecpp = library(
        'sqlitecpp',
        sqlitecpp_srcs,
        include_directories: sqlitecpp_incl,
        cpp_args: sqlitecpp_args,
        dependencies: sqlitecpp_deps,
        # override the default options
        override_options: sqlitecpp_opts,
        # install: true,
        # API version for SQLiteCpp shared library.
        version: '0',)
endif

if get_option('SQLITECPP_BUILD_TESTS')
    # for the unit tests we need to link against a static version of SQLiteCpp
    if host_machine.system() == 'windows' or get_option('default_library') == 'static'
        # we do not need to recomplile the library
        libsqlitecpp_static = libsqlitecpp
    else
        libsqlitecpp_static = static_library(
            'sqlitecpp_static',
            sqlitecpp_srcs,
            include_directories: sqlitecpp_incl,
            cpp_args: sqlitecpp_args,
            dependencies: sqlitecpp_deps,
            # override the default options
            override_options: sqlitecpp_opts,)
    endif
endif

install_subdir(
    'include/SQliteCpp',
    install_dir: get_option('includedir'))

sqlitecpp_dep = declare_dependency(
    include_directories: sqlitecpp_incl,
    link_with: libsqlitecpp,
)
if get_option('SQLITECPP_BUILD_TESTS')
    ## make the dependency static so the unit tests can link against it
    ## (mainly for windows as the symbols are not exported by default)
    sqlitecpp_static_dep = declare_dependency(
        include_directories: sqlitecpp_incl,
        link_with: libsqlitecpp_static,
    )
endif

if get_option('SQLITECPP_BUILD_TESTS')
    gtest_dep = dependency(
                'gtest',
                main : true,
                fallback: ['gtest', 'gtest_main_dep'])
    sqlitecpp_test_dependencies = [
        gtest_dep,
        sqlitecpp_static_dep,
        sqlite3_dep,
    ]

    testexe = executable('testexe', sqlitecpp_test_srcs,
                     dependencies: sqlitecpp_test_dependencies,
                     cpp_args: sqlitecpp_test_args,
                     # override the default options
                     override_options: sqlitecpp_opts,)

    test_args = []

    test('sqlitecpp unit tests', testexe, args: test_args)
endif
if get_option('SQLITECPP_BUILD_EXAMPLES')
    ## demo 1 executable
    sqlitecpp_demo1_exe = executable('SQLITECPP_sample_demo1',
                            sqlitecpp_sample1_srcs,
                            dependencies: sqlitecpp_dep,
                            # override the default options
                            override_options: sqlitecpp_opts,)
    ## demo 2 executable
    sqlitecpp_demo1_exe = executable('SQLITECPP_sample_demo2',
                            sqlitecpp_sample2_srcs,
                            dependencies: sqlitecpp_dep,
                            # override the default options
                            override_options: sqlitecpp_opts,)
                    
endif

pkgconfig = import('pkgconfig')
pkgconfig.generate(
    libsqlitecpp,
    description: 'a smart and easy to use C++ SQLite3 wrapper.',
    version: meson.project_version(),
)
