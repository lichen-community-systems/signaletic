project(
    'libstar',
    'c',
    default_options: ['c_std=c99']
)
project_description = 'A very portable music signal processing library.'

headers = include_directories('include', 'vendor'/'tlsf')

source_files = [
    'vendor'/'tlsf'/'tlsf.c',
    'src'/'libstar.c'
]

test_files = [
    'tests'/'test-libstar.c'
]

# libstar
libstar_library = static_library(
    meson.project_name(),
    source_files,
    include_directories: headers,
    link_args: '-lm'
)

libstar_dep = declare_dependency(
    include_directories: headers,
    link_with: libstar_library
)

# Meson only seems to produce .wasm and .js files for executables
if host_machine.system() == 'emscripten'
    # Compile as C++ when using WebIDL.
    add_languages('cpp')

    executable(
        'libstar',
        [
            'wasm'/'bindings'/'src'/'libstar-web-wrapper.cpp'
        ],
        dependencies: [libstar_dep],
        link_args: [
            '--no-entry',
            '--post-js', '..'/'bindings'/'libstar-web-bindings.js'
        ]
    )
endif

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
        unity_dir/'src'
    ),
    link_with: static_library(
        'unity',
        sources: files(unity_dir/'src'/'unity.c')
    )
)

test('all_tests',
    executable(
        'run_tests',
        files(test_files),
        dependencies: [libstar_dep, unity_dep],
        install: false,
        link_args: '-lm'
    )
)
