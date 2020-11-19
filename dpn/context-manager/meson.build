inc = include_directories('inc/')
src = files([
    'src/ContextManager.cpp'
])


context_manager_lib = library(
    'context-manager', 
    src, 
    include_directories: [inc], 
    dependencies: [cppzmq_dep, util_dep]
)

context_manager_dep = declare_dependency(
    include_directories: inc, 
    link_with: context_manager_lib, 
    dependencies: [cppzmq_dep, util_dep]
)

subdir('test/')