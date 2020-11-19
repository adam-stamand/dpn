project('spdlog', 'cpp')

add_project_arguments('-DSPDLOG_COMPILED_LIB', language:'cpp')

_src = files([
    'spdlog/src/spdlog.cpp', 
    'spdlog/src/stdout_sinks.cpp', 
    'spdlog/src/color_sinks.cpp', 
    'spdlog/src/file_sinks.cpp', 
    'spdlog/src/async.cpp', 
    'spdlog/src/cfg.cpp',
    'spdlog/src/fmt.cpp'
])

_inc = include_directories([
    'spdlog/include'
])

_lib = static_library(
    meson.project_name(),
    sources: [_src],
    include_directories: _inc
)

spdlog_dep = declare_dependency(
    link_with: [
        _lib
    ],
    include_directories: _inc
)
