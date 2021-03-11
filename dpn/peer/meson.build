inc = include_directories('inc/')
src = files([
    'src/Peer.cpp',
])

_args = [
    '-DZMQ_BUILD_DRAFT_API'
]

peer_lib = static_library(
    'peer', 
    src, 
    include_directories: [inc], 
    dependencies: [
        hub_dep,
        label_dep,
        port_dep,
        util_dep,  
        interface_dep],
    cpp_args: _args
)

peer_dep = declare_dependency(
    include_directories: inc, 
    link_with: peer_lib, 
    dependencies: [
        hub_dep,
        label_dep,
        port_dep,
        util_dep,  
        interface_dep])

subdir('test/')