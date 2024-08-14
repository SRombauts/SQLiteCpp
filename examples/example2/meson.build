example2_srcs = files('src/main.cpp')

# if running on windows define _CRT_SECURE_NO_WARNINGS
example2_args = []

executable(
    'SQLITECPP_sample_demo2',
    example2_srcs,
    dependencies: sqlitecpp_dep,
    # inherit the default options from sqlitecpp
    override_options: sqlitecpp_opts,
    cpp_args: example2_args,
)
