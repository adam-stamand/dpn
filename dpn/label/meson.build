inc = include_directories('inc/')
src = files([
    'src/Message.cpp',
])

message_lib = library(
    'message', 
    src, 
    include_directories: [inc], 
    dependencies: [cppzmq_dep, util_dep]
)

message_dep = declare_dependency(
    include_directories: inc, 
    link_with: message_lib, 
    dependencies: [cppzmq_dep, util_dep]
)

# subdir('test/')