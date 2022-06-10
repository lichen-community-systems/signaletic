project(
    'libsignaletic',
    'c',
    default_options: ['c_std=c99']
)
project_description = 'A very portable music signal processing library.'

headers = include_directories('include', 'vendor'/'tlsf')

source_files = [
    'vendor'/'tlsf'/'tlsf.c',
    'src'/'libsignaletic.c'
]

test_files = [
    'tests'/'util'/'buffer-test-utils.c',
    'tests'/'test-libsignaletic.c'
]

# libsignaletic
libsig_library = static_library(
    meson.project_name(),
    source_files,
    include_directories: headers,
    link_args: '-lm'
)

libsig_dep = declare_dependency(
    include_directories: headers,
    link_with: libsig_library
)

# Meson only seems to produce .wasm and .js files for executables
if host_machine.system() == 'emscripten'
    # Compile as C++ when using WebIDL.
    add_languages('cpp')

    executable(
        'libsignaletic',
        [
            'wasm'/'bindings'/'src'/'libsignaletic-web-wrapper.cpp'
        ],
        dependencies: [libsig_dep],
        link_args: [
            '--no-entry',
            '--post-js', '..'/'bindings'/'libsignaletic-web-bindings.js'
        ]
    )

    executable(
        'libsignaletic-worklet',
        [
            'wasm'/'bindings'/'src'/'libsignaletic-web-wrapper.cpp'
        ],
        dependencies: [libsig_dep],
        link_args: [
            '--no-entry',
            '-s', 'ENVIRONMENT=shell',
            '-s', 'WASM_ASYNC_COMPILATION=0',
            '--post-js', '..'/'bindings'/'libsignaletic-web-bindings.js'
        ]
    )

    executable(
        'wasm-oscillator-example',
        [
            'wasm'/'bindings'/'src'/'libsignaletic-web-wrapper.cpp'
        ],
        dependencies: [libsig_dep],
        link_args: [
            '--no-entry',
            '-s', 'ENVIRONMENT=shell',
            '-s', 'WASM_ASYNC_COMPILATION=0',
            '--post-js', '..'/'bindings'/'libsignaletic-web-bindings.js',
            '--post-js', '..'/'..'/'..'/'hosts'/'web'/'examples'/'oscillator'/'signaletic-oscillator-worklet.js'
        ]
    )
endif

# Examples
executable(
    'libsignaletic-console-example',
    'examples'/'console'/'src'/'print-sine.c',
    dependencies: [libsig_dep],
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
        include_directories: 'tests'/'util'/'include',
        dependencies: [libsig_dep, unity_dep],
        install: false,
        link_args: '-lm'
    )
)
