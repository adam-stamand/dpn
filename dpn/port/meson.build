inc = include_directories('inc/')
src = files([
    'src/Port.cpp',
])

port_lib = library(
    'port', 
    src, 
    include_directories: [inc], 
    dependencies: [cppzmq_dep, util_dep, message_dep]
)

port_dep = declare_dependency(
    include_directories: inc, 
    link_with: port_lib, 
    dependencies: [cppzmq_dep, util_dep, message_dep]
)

subdir('test/')