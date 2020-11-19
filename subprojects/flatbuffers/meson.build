project('flatc', 'cpp')

_inc = include_directories([
    'flatbuffers/include/',
    'flatbuffers/grpc/'
])

_src = files([
  'flatbuffers/src/idl_parser.cpp',
  'flatbuffers/src/idl_gen_text.cpp',
  'flatbuffers/src/reflection.cpp',
  'flatbuffers/src/util.cpp',
  'flatbuffers/src/idl_gen_cpp.cpp',
  'flatbuffers/src/idl_gen_csharp.cpp',
  'flatbuffers/src/idl_gen_dart.cpp',
  'flatbuffers/src/idl_gen_kotlin.cpp',
  'flatbuffers/src/idl_gen_go.cpp',
  'flatbuffers/src/idl_gen_java.cpp',
  'flatbuffers/src/idl_gen_js_ts.cpp',
  'flatbuffers/src/idl_gen_php.cpp',
  'flatbuffers/src/idl_gen_python.cpp',
  'flatbuffers/src/idl_gen_lobster.cpp',
  'flatbuffers/src/idl_gen_lua.cpp',
  'flatbuffers/src/idl_gen_rust.cpp',
  'flatbuffers/src/idl_gen_fbs.cpp',
  'flatbuffers/src/idl_gen_grpc.cpp',
  'flatbuffers/src/idl_gen_json_schema.cpp',
  'flatbuffers/src/idl_gen_swift.cpp',
  'flatbuffers/src/flatc.cpp',
  'flatbuffers/src/flatc_main.cpp',
  'flatbuffers/src/code_generators.cpp',
#   'flatbuffers/src/flathash.cpp',
  'flatbuffers/grpc/src/compiler/cpp_generator.cc',
  'flatbuffers/grpc/src/compiler/go_generator.cc',
  'flatbuffers/grpc/src/compiler/java_generator.cc',
  'flatbuffers/grpc/src/compiler/python_generator.cc',
  'flatbuffers/grpc/src/compiler/swift_generator.cc',
  'flatbuffers/grpc/src/compiler/ts_generator.cc'
])


_dep = [
    dependency('threads'),
]


_flatc_exe = executable(
    meson.project_name(),
    sources: [_src],
    include_directories: _inc,
    dependencies: [_dep],
)

# _flatc_exe = executable(
#     link_with: [
#         _lib
#     ],
#     include_directories: _inc,
#     dependencies: [_dep]
# )

# _flatc_exe = executable(
#     'flatc', # keep naming convention from capnp project
#     sources: [
#         _capnp_project_root_langcpp / 'capnp/compiler/capnpc-c++.c++',
#     ],
#     dependencies: capnp_dep_langcpp
# )


flatc_gen = generator(
    _flatc_exe,
    output: [
        '@BASENAME@.h'
    ],
    arguments: [
        '--cpp',
        '-o',
        '@BUILD_DIR@',
        # '-o',
        # _flatc_exe.full_path() + ':@BUILD_DIR@',
        # '-I' + meson.current_source_dir() / 'global',
        '@EXTRA_ARGS@',
        '@INPUT@'
    ],
    depends: _flatc_exe
)


flatc_dep = declare_dependency(
    include_directories: 'flatbuffers/include/'
)