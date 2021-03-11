subdir('util/')
subdir('message/')
subdir('interface/')
subdir('context-manager/')
subdir('port/')
subdir('hub/')
subdir('label/')
subdir('peer/')

_inc = include_directories([
    '.',
])


dpn_dep = declare_dependency(
    include_directories: _inc, 
    dependencies: [
        cppzmq_dep, 
        message_dep,
        hub_dep, 
        label_dep,
        port_dep, 
        context_manager_dep,
        util_dep,
        peer_dep,
        interface_dep
        ],
)
