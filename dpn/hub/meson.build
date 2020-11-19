inc = include_directories('inc/')
src = files([
    'src/Hub.cpp'
])

_args = [
    '-DZMQ_BUILD_DRAFT_API'
]

hub_lib = library(
    'hub', 
    src, 
    include_directories: [inc], 
    dependencies: [port_dep],
    cpp_args: _args
)

hub_dep = declare_dependency(
    include_directories: inc, 
    link_with: hub_lib, 
    dependencies: [port_dep]
)

subdir('test/')