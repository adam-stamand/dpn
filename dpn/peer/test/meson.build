_args = [
    '-DZMQ_BUILD_DRAFT_API'
]

_dep = [
    dependency('threads')
]


test(
    'peer.test',
    executable('peer.test', ['test.cpp'], dependencies: [gtest_dep, peer_dep, _dep], cpp_args: [_args]),
    args: [gtest_args,_args],
    suite: 'peer'
)