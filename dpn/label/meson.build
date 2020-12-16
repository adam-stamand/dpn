inc = include_directories('inc/')
src = files([
])

label_lib = library(
    'label', 
    src, 
    include_directories: [inc], 
    dependencies: [cppzmq_dep, util_dep]
)

label_dep = declare_dependency(
    include_directories: inc, 
    link_with: label_lib, 
    dependencies: [cppzmq_dep, util_dep]
)

# subdir('test/')