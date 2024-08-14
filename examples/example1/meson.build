
example1_sources = files('main.cpp')

example1_args = []

## under windows define _CRT_SECURE_NO_WARNINGS
if host_machine.system() == 'windows'
    example1_args += ['-D_CRT_SECURE_NO_WARNINGS']
endif

executable(
    'SQLITECPP_sample_demo1',
    sources: example1_sources,
    dependencies: sqlitecpp_dep,
    # inherit the default options from sqlitecpp
    override_options: sqlitecpp_opts,
    cpp_args: example1_args,
)
