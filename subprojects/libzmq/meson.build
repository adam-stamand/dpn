project('zmq', 'cpp')

# Get host machine specific configuration
subdir('config/')

# Grab necessary variables from system configuration
_system_config = {
    'config_file'   : _zmq_system_dep.get_variable(internal: 'config_file')
}


_platform_headers = custom_target(
    'platform.hpp',
    output: 'platform.hpp',
    command: [
        find_program('cmake_configure_file.sh', dirs: [meson.source_root() / 'meson/scripts/']),
        meson.current_source_dir() / 'libzmq', # CMake source dir
        '@PRIVATE_DIR@',                       # where to put intermediate files
        '@PRIVATE_DIR@/platform.hpp',          # Source file to copy
        '@OUTPUT@',                            # Copy destination
        '-C ' + meson.current_source_dir() / _system_config['config_file'], # Source the system config.cmake to initialize the project settings
    ]
)


_src = files([
    'libzmq/src/address.cpp',
    'libzmq/src/channel.cpp',
    'libzmq/src/client.cpp',
    'libzmq/src/clock.cpp',
    'libzmq/src/ctx.cpp',
    # 'libzmq/src/curve_client.cpp',
    # 'libzmq/src/curve_mechanism_base.cpp',
    # 'libzmq/src/curve_server.cpp',
    'libzmq/src/dealer.cpp',
    'libzmq/src/decoder_allocators.cpp',
    'libzmq/src/devpoll.cpp',
    'libzmq/src/dgram.cpp',
    'libzmq/src/dish.cpp',
    'libzmq/src/dist.cpp',
    'libzmq/src/endpoint.cpp',
    'libzmq/src/epoll.cpp',
    'libzmq/src/err.cpp',
    'libzmq/src/fq.cpp',
    'libzmq/src/gather.cpp',
    # 'libzmq/src/gssapi_client.cpp',
    # 'libzmq/src/gssapi_mechanism_base.cpp',
    # 'libzmq/src/gssapi_server.cpp',
    'libzmq/src/io_object.cpp',
    'libzmq/src/io_thread.cpp',
    'libzmq/src/ip.cpp',
    'libzmq/src/ip_resolver.cpp',
    'libzmq/src/ipc_address.cpp',
    'libzmq/src/ipc_connecter.cpp',
    'libzmq/src/ipc_listener.cpp',
    'libzmq/src/kqueue.cpp',
    'libzmq/src/lb.cpp',
    # 'libzmq/src/libzmq.vers',
    # 'libzmq/src/libzmq.pc.in',
    'libzmq/src/mailbox.cpp',
    'libzmq/src/mailbox_safe.cpp',
    'libzmq/src/mechanism.cpp',
    'libzmq/src/mechanism_base.cpp',
    'libzmq/src/metadata.cpp',
    'libzmq/src/msg.cpp',
    'libzmq/src/mtrie.cpp',
    'libzmq/src/norm_engine.cpp',
    'libzmq/src/null_mechanism.cpp',
    'libzmq/src/object.cpp',
    'libzmq/src/options.cpp',
    'libzmq/src/own.cpp',
    'libzmq/src/pair.cpp',
    'libzmq/src/peer.cpp',
    'libzmq/src/pgm_receiver.cpp',
    'libzmq/src/pgm_sender.cpp',
    'libzmq/src/pgm_socket.cpp',
    'libzmq/src/pipe.cpp',
    'libzmq/src/plain_client.cpp',
    'libzmq/src/plain_server.cpp',
    'libzmq/src/poll.cpp',
    'libzmq/src/poller_base.cpp',
    'libzmq/src/polling_util.cpp',
    'libzmq/src/pollset.cpp',
    'libzmq/src/precompiled.cpp',
    'libzmq/src/proxy.cpp',
    'libzmq/src/pub.cpp',
    'libzmq/src/pull.cpp',
    'libzmq/src/push.cpp',
    'libzmq/src/radio.cpp',
    'libzmq/src/radix_tree.cpp',
    'libzmq/src/random.cpp',
    'libzmq/src/raw_decoder.cpp',
    'libzmq/src/raw_encoder.cpp',
    'libzmq/src/raw_engine.cpp',
    'libzmq/src/reaper.cpp',
    'libzmq/src/rep.cpp',
    'libzmq/src/req.cpp',
    'libzmq/src/router.cpp',
    'libzmq/src/scatter.cpp',
    'libzmq/src/select.cpp',
    'libzmq/src/server.cpp',
    'libzmq/src/session_base.cpp',
    'libzmq/src/signaler.cpp',
    'libzmq/src/socket_base.cpp',
    'libzmq/src/socket_poller.cpp',
    'libzmq/src/socks.cpp',
    'libzmq/src/socks_connecter.cpp',
    'libzmq/src/stream.cpp',
    'libzmq/src/stream_connecter_base.cpp',
    'libzmq/src/stream_engine_base.cpp',
    'libzmq/src/stream_listener_base.cpp',
    'libzmq/src/sub.cpp',
    'libzmq/src/tcp.cpp',
    'libzmq/src/tcp_address.cpp',
    'libzmq/src/tcp_connecter.cpp',
    'libzmq/src/tcp_listener.cpp',
    'libzmq/src/thread.cpp',
    'libzmq/src/timers.cpp',
    'libzmq/src/tipc_address.cpp',
    'libzmq/src/tipc_connecter.cpp',
    'libzmq/src/tipc_listener.cpp',
    'libzmq/src/trie.cpp',
    # 'libzmq/src/tweetnacl.c',
    # 'libzmq/src/tweetnacl.h',
    'libzmq/src/udp_address.cpp',
    'libzmq/src/udp_engine.cpp',
    'libzmq/src/v1_decoder.cpp',
    'libzmq/src/v1_encoder.cpp',
    'libzmq/src/v2_decoder.cpp',
    'libzmq/src/v2_encoder.cpp',
    'libzmq/src/v3_1_encoder.cpp',
    # 'libzmq/src/version.rc.in',
    # 'libzmq/src/vmci.cpp',
    # 'libzmq/src/vmci_address.cpp',
    # 'libzmq/src/vmci_connecter.cpp',
    # 'libzmq/src/vmci_listener.cpp',
    # 'libzmq/src/ws_address.cpp',
    # 'libzmq/src/ws_connecter.cpp',
    # 'libzmq/src/ws_decoder.cpp',
    # 'libzmq/src/ws_encoder.cpp',
    # 'libzmq/src/ws_engine.cpp',
    # 'libzmq/src/ws_listener.cpp',
    # 'libzmq/src/wss_address.cpp',
    # 'libzmq/src/wss_engine.cpp',
    'libzmq/src/xpub.cpp',
    'libzmq/src/xsub.cpp',
    'libzmq/src/zap_client.cpp',
    'libzmq/src/zmq.cpp',
    'libzmq/src/zmq_draft.h',
    'libzmq/src/zmq_utils.cpp',
    'libzmq/src/zmtp_engine.cpp',
])

_inc = include_directories([
    'libzmq/include'
])

_dep = [
    dependency('threads'),
]

_args = [
    '-DZMQ_CUSTOM_PLATFORM_HPP'
]


_lib = static_library(
    meson.project_name(),
    sources: [_src,_platform_headers],
    include_directories: _inc,
    dependencies: [_dep],
    cpp_args: _args
)

zmq_dep = declare_dependency(
    link_with: [
        _lib
    ],
    include_directories: _inc,
    dependencies: [_dep]
)
