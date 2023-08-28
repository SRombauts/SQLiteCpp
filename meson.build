project(
    'SQLiteCpp', 'cpp',
    # SQLiteCpp supports C++11
    # however newer versions of gtest requires C++14
    default_options: ['cpp_std=c++14', 'warning_level=3'],
    license: 'MIT',
    version: '3.3.1',
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

mingw_64_env_code = '''
    #if defined(__MINGW64__)
    // do nothing
    #else
    # error "Non MinGW-W64 environment"
    #endif
'''
mingw_64_env = cxx.compiles(mingw_64_env_code, name : 'MinGW-W64 environment')

thread_dep = dependency('threads')
# sqlite3 support
sqlite3_dep = dependency(
                'sqlite3',
                fallback: ['sqlite3', 'sqlite3_dep']
)

sqlitecpp_incl = [ 
    include_directories('include')
]
sqlitecpp_srcs = files(
    'src/Backup.cpp',
    'src/Column.cpp',
    'src/Database.cpp',
    'src/Exception.cpp',
    'src/Savepoint.cpp',
    'src/Statement.cpp',
    'src/Transaction.cpp',
)
sqlitecpp_args = cxx.get_supported_arguments(
    # included in meson by default
    # -Wall
    # included when warning_level=3
    #'-Wextra',
    #'-Wpedantic',
    '-Wswitch-enum',
    '-Wshadow',
    '-Wno-long-long',
    '-Wno-attributes',
)
sqlitecpp_link = []
sqlitecpp_deps = [
    sqlite3_dep,
    thread_dep,
]
## used to override the default sqlitecpp options like cpp standard
sqlitecpp_opts = []

## used to set required macros when using sqlitecpp
sqlitecpp_dep_args = []

## tests

sqlitecpp_test_srcs = files(
    'tests/Column_test.cpp',
    'tests/Database_test.cpp',
    'tests/Savepoint_test.cpp',
    'tests/Statement_test.cpp',
    'tests/Backup_test.cpp',
    'tests/Transaction_test.cpp',
    'tests/VariadicBind_test.cpp',
    'tests/Exception_test.cpp',
    'tests/ExecuteMany_test.cpp',
)
sqlitecpp_test_args = []

## samples

sqlitecpp_sample1_srcs = files(
    'examples/example1/main.cpp',
)
sqlitecpp_sample2_srcs = files(
    'examples/example2/src/main.cpp',
)

## using MSVC headers requires c++14, if not will show an error on xstddef as: 
## 'auto' return without trailing return type; deduced return types are a C++14 extension
if host_machine.system() == 'windows'
    ## check if the std version is less than c++14
    if get_option('cpp_std').endswith('11')
        message('[WINDOWS] using c++14 standard')
        message('[WINDOWS] you can disable this warning by setting cpp_std to c++14 or newer')
        sqlitecpp_opts += [
            'cpp_std=c++14',
        ]
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
        '-DSQLITE_HAS_CODEC',
    ]
endif

if get_option('SQLITE_USE_LEGACY_STRUCT')
    sqlitecpp_args += [
        '-DSQLITE_USE_LEGACY_STRUCT',
    ] 
endif

## C++17 disable the support for std::filesystem (by default off)
if get_option('SQLITECPP_DISABLE_STD_FILESYSTEM')
    sqlitecpp_cxx_flags += ['-DSQLITECPP_DISABLE_STD_FILESYSTEM']
endif

## get the user option for the SQLITECPP_DISABLE_SQLITE3_EXPANDED_SQL
disable_sqlitecpp_expanded_sql = get_option('SQLITECPP_DISABLE_SQLITE3_EXPANDED_SQL')

## Disable the use of sqlite3_expanded_sql (from sqlite3 3.14.0)
if disable_sqlitecpp_expanded_sql
    sqlitecpp_args += ['-DSQLITECPP_DISABLE_SQLITE3_EXPANDED_SQL']
endif

## stack protection hardening
if get_option('SQLITECPP_USE_STACK_PROTECTION')
    ## if is on MinGW-W64 give a warning that is not supported
    if mingw_64_env
        message('warning: SQLiteCpp does not support stack protection on MinGW-W64')
        message('warning: this could lead to a crash on the application')
        message('warning: you can disable this warning by setting SQLITECPP_USE_STACK_PROTECTOR to false')
    ## check if it is supported by the compiler
    elif cxx.has_argument('-fstack-protector')
        sqlitecpp_args += ['-fstack-protector']
    ## if not supported give a warning
    else
        message('warning: SQLiteCpp does not have stack protection support in this compiler')
        message('warning: this argument will be ignored')
        message('warning: you can disable this warning by setting SQLITECPP_USE_STACK_PROTECTOR to false')
    endif
endif

## enable ommit load extension
if get_option('SQLITE_OMIT_LOAD_EXTENSION')
    sqlitecpp_args += ['-DSQLITE_OMIT_LOAD_EXTENSION']
## check if running on OSX
elif host_machine.system() == 'darwin' and sqlite3_dep.found()
    ## check if sqlite3 is the one bundled with OSX
    if sqlite3_dep.type_name() != 'internal'
        message('warning: Detected non-internal SQLite3 in OSX, check if it supports load extension')
        sqlite3_load_extension_support = cxx.links(
            '''
                #include <sqlite3.h>
                int main() {
                    sqlite3_enable_load_extension(0, 0);
                    return 0;
                }
            ''', 
            name: 'sqlite3_load_extension',
            dependencies: [sqlite3_dep])
        if not sqlite3_load_extension_support
            message('warning: Detected bundled SQLite3 in OSX, but it does not support load extension')
            message('warning: SQLiteCpp will be built without load extension support')
            message('warning: You can disable this warning by setting SQLITE_OMIT_LOAD_EXTENSION to false')
            sqlitecpp_args += ['-DSQLITE_OMIT_LOAD_EXTENSION']
        endif
    endif
endif



if unix_like
    sqlitecpp_args += [
        # -fPIC is included by default in meson
        # 'fPIC',
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

sqlitecpp_static_args = sqlitecpp_args
sqlitecpp_static_dep_args = sqlitecpp_dep_args

# if windows and shared library
if host_machine.system() == 'windows' and get_option('default_library') == 'shared'
    # compile with SQLITECPP_COMPILE_DLL and SQLITECPP_DLL_EXPORT=1
    sqlitecpp_args += [
        '-DSQLITECPP_COMPILE_DLL',
        '-DSQLITECPP_DLL_EXPORT',
    ]
    sqlitecpp_dep_args += [
        # we just need to define SQLITECPP_COMPILE_DLL
        '-DSQLITECPP_COMPILE_DLL',
    ]
endif


libsqlitecpp = library(
    'sqlitecpp',
    sqlitecpp_srcs,
    include_directories: sqlitecpp_incl,
    cpp_args: sqlitecpp_args,
    dependencies: sqlitecpp_deps,
    # override the default options
    override_options: sqlitecpp_opts,
    install: true,
    # API version for SQLiteCpp shared library.
    version: '0',)



if get_option('SQLITECPP_BUILD_TESTS')
    # for the unit tests we need to link against a static version of SQLiteCpp
    if get_option('default_library') == 'static'
        # we do not need to recomplile the library
        libsqlitecpp_static = libsqlitecpp
    else
        libsqlitecpp_static = static_library(
            'sqlitecpp_static',
            sqlitecpp_srcs,
            include_directories: sqlitecpp_incl,
            cpp_args: sqlitecpp_static_args,
            dependencies: sqlitecpp_deps,
            # override the default options
            override_options: sqlitecpp_opts,)
    endif
endif

install_subdir(
    'include/SQLiteCpp',
    install_dir: get_option('includedir'))

sqlitecpp_dep = declare_dependency(
    include_directories: sqlitecpp_incl,
    link_with: libsqlitecpp,
    compile_args: sqlitecpp_dep_args,
)
if get_option('SQLITECPP_BUILD_TESTS')
    ## make the dependency static so the unit tests can link against it
    ## (mainly for windows as the symbols are not exported by default)
    sqlitecpp_static_dep = declare_dependency(
        include_directories: sqlitecpp_incl,
        link_with: libsqlitecpp_static,
        compile_args: sqlitecpp_static_dep_args,
    )
endif

if get_option('SQLITECPP_BUILD_TESTS')
    gtest_dep = dependency(
                'gtest',
                main : true,
                fallback: ['gtest', 'gtest_main_dep'])
    # check if Gtest is newer than 1.14.0
    # if so set the minimum standard to c++14
    if gtest_dep.found() and gtest_dep.version().version_compare('>=1.14.0')
        ## check if the std version is less than c++14
        if get_option('cpp_std').version_compare('<c++14')
            message('warning: current Gtest version requires c++14, setting the minimum standard to c++14')
            message('warning: you can disable this warning by setting cpp_std to c++14 or newer')

            sqlitecpp_test_args += [
                'cpp_std=c++14',
            ]
        endif
    endif


    sqlitecpp_test_dependencies = [
        gtest_dep,
        sqlitecpp_dep,
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
    subdir('examples')
endif

pkgconfig = import('pkgconfig')
pkgconfig.generate(
    libsqlitecpp,
    description: 'a smart and easy to use C++ SQLite3 wrapper.',
    version: meson.project_version(),
)
