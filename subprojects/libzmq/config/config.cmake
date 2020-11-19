################################################################################
## Configuration Options ##
# These change which features will be enabled.
################################################################################
set(API_POLLER  "" CACHE STRING "Choose polling system for zmq_poll(er)_*. valid values are poll or select [default=poll unless POLLER=select]")
set(POLLER      "" CACHE STRING "Choose polling system for I/O threads. valid values are kqueue, epoll, devpoll, pollset, poll or select [default=autodetect]")
# set(ZMQ_CV_IMPL "" CACHE STRING "Choose condition_variable_t implementation. Valid values are stl11, win32api, pthreads, none [default=autodetect]")

option(ENABLE_RADIX_TREE     "Use radix tree implementation to manage subscriptions" OFF)
option(WITH_OPENPGM          "Build with support for OpenPGM                       " OFF)
option(WITH_NORM             "Build with support for NORM                          " OFF)
option(WITH_VMCI             "Build with support for VMware VMCI socket            " OFF)
option(ENABLE_DRAFTS         "Build and install draft classes and methods          "  ON)
option(ENABLE_WS             "Enable WebSocket transport                           " OFF)
option(WITH_TLS              "Use TLS for WSS support                              " OFF)
option(WITH_NSS              "Use NSS instead of builtin sha1                      " OFF)
option(WITH_LIBBSD           "Use libbsd instead of builtin strlcpy                " OFF)
option(WITH_LIBSODIUM        "Use libsodium instead of built-in tweetnacl          " OFF)
option(WITH_LIBSODIUM_STATIC "Use static libsodium library                         " OFF)
option(ENABLE_CURVE          "Enable CURVE security                                " OFF)
option(WITH_MILITANT         "Enable militant assertions                           " OFF)
option(ENABLE_ANALYSIS       "Build with static analysis(make take very long)      " OFF)

################################################################################
## Build Options ##
# These probably don't do anything, since we are not using cmake to build.
################################################################################

option(LIBZMQ_PEDANTIC    "                                                     " OFF)
option(LIBZMQ_WERROR      "                                                     " OFF)

option(ENABLE_ASAN        "Build with address sanitizer                         " OFF)
option(ENABLE_TSAN        "Build with thread sanitizer                          " OFF)
option(ENABLE_INTRINSICS  "Build using compiler intrinsics for atomic ops       " OFF)
option(WITH_DOCS          "Build html docs                                      " OFF)
option(ENABLE_PRECOMPILED "Enable precompiled headers, if possible              " OFF)
option(BUILD_SHARED       "Whether or not to build the shared object            " OFF)
option(BUILD_STATIC       "Whether or not to build the static archive           " OFF)
option(WITH_PERF_TOOL     "Build with perf-tools                                " OFF)
option(BUILD_TESTS        "Whether or not to build the tests                    " OFF)
option(ENABLE_CPACK       "Enables cpack rules                                  " OFF)
option(ENABLE_CLANG       "Include Clang                                        " OFF)
option(ZMQ_BUILD_FRAMEWORK   "Build as OS X framework                           " OFF)
