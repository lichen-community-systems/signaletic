project(
    'libstar',
    'c',
    default_options: ['c_std=c89']
)
project_description = 'A very portable music signal processing library.'

headers = include_directories('include')

source_files = [
    'src/libstar.c'
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
    'examples/console/src/print-sine.c',
    dependencies: [libstar_dep],
    link_args: '-lm'
)
