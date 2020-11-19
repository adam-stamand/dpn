inc = include_directories('inc/')
src = files([
])

interface_lib = static_library('interface', src, include_directories: [inc], dependencies: [message_dep])
interface_dep = declare_dependency(include_directories: inc, link_with: interface_lib, dependencies: [message_dep])

# subdir('test/')