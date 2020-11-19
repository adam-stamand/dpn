project('cppzmq', 'cpp')



_inc = include_directories([
    'cppzmq/'
])

_dep = [
    subproject('libzmq').get_variable('zmq_dep')
]


_lib = static_library(
    meson.project_name(),
    include_directories: _inc,
    dependencies: [_dep]
)

cppzmq_dep = declare_dependency(
    link_with: _lib,
    include_directories: _inc,
    dependencies: _dep
)
