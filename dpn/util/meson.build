inc = include_directories('inc/')
src = files([
    'src/DPNLogger.cpp'
])


util_lib = library(
    'util', 
    src, 
    include_directories: [inc], 
    dependencies: [spdlog_dep]
)

util_dep = declare_dependency(
    include_directories: inc, 
    link_with: util_lib, 
    dependencies: spdlog_dep
)

# subdir('test/')