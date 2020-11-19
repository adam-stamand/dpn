_args = [
    '-DZMQ_BUILD_DRAFT_API',
]


test(
    'hub.test',
    executable('hub.test', ['test.cpp'], dependencies: [gtest_dep, hub_dep], cpp_args: [_args]),
    args: [gtest_args],
    suite: 'hub'
)