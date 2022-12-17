

sqlitecpp_dep = dependency(
                'sqlitecpp_dep',
                fallback: ['SQLiteCpp', 'sqlitecpp_dep']
)

example1_sources = files(
    'src/main.cpp'
)

example1_dependencies = [
    dependency('SQLite')
]

sqlitecpp_demo1_exe = executable('SQLITECPP_sample_demo1',
                            sqlitecpp_sample1_srcs,
                            dependencies: sqlitecpp_dep,
                            # override the default options
                            override_options: sqlitecpp_opts,)