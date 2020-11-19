project('gtest', 'cpp')

_inc = include_directories([
    'googletest/googletest/include',
    'googletest/googlemock/include'
])

_private = include_directories([
    'googletest/googletest',
    'googletest/googlemock'
])

_src = files([
    'googletest/googletest/src/gtest-all.cc',
    'googletest/googlemock/src/gmock-all.cc',
    'googletest/googlemock/src/gmock_main.cc'
])

_dep = [
    dependency('threads')
]

_lib = library(
    meson.project_name(),
    sources: _src,
    include_directories: [_inc, _private],
    dependencies: _dep
)

gtest_dep = declare_dependency(
    link_with: _lib,
    include_directories: _inc,
    dependencies: _dep
)
