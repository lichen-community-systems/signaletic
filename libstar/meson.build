project(
    'libstar',
    'c',
    default_options: ['c_std=c99']
)
project_description = 'A very portable music signal processing library.'

headers = include_directories('include')

source_files = [
    'src'/'libstar.c'
]

test_files = [
    'tests'/'test-core-functions.c'
]

# libstar
libstar_library = static_library(
    meson.project_name(),
    source_files,
    link_args: '-lm'
)

libstar_dep = declare_dependency(
    include_directories: headers,
    link_with: libstar_library
)

# Examples
executable(
    'libstar-console-example',
    'examples'/'console'/'src'/'print-sine.c',
    dependencies: [libstar_dep],
    link_args: '-lm'
)

# Tests
unity_dir = 'tests'/'vendor'/'unity'

unity_dep = declare_dependency(
    include_directories: include_directories(
        unity_dir / 'src'
    ),
    link_with: static_library(
        'unity',
        sources: files(unity_dir / 'src' / 'unity.c')
    )
)

test('all_tests',
    executable(
        'run_tests',
        files(test_files),
        dependencies : [libstar_dep, unity_dep],
        install : false
    )
)
