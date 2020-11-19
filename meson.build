project('dpn', 'c', 'cpp', default_options: [ 'c_std=c11', 'cpp_std=c++11'])

add_project_arguments('-DDPN_LOG_LEVEL=DPN_LOG_LEVEL_TRACE', language : 'cpp')
add_project_arguments('-DDPN_LOGGING_ENABLED=1', language : 'cpp')

# Build subprojects
#
# These *_dep objects can be used as dependency arguments in any of our targets. This will automatically add the
# include paths during compilation, and link the target against any inherited dependencies. Dependency objects can also
# add compile/link-flags flags, see the documentation on dependency object for more details
#
# Note: Subprojects can reference each other since all subprojects in Meson are relative to the root project, although
# this is rare. See zmqpp as an example.
gtest_dep = subproject('googletest').get_variable('gtest_dep')

libzmq_dep  = subproject('libzmq').get_variable('zmq_dep')
cppzmq_dep   = subproject('cppzmq').get_variable('cppzmq_dep')
spdlog_dep   = subproject('spdlog').get_variable('spdlog_dep')

# flatc_gen = subproject('flatbuffers').get_variable('flatc_gen')
# flatc_dep = subproject('flatbuffers').get_variable('flatc_dep')

gtest_args = ['--gtest_color=yes', '--gtest_shuffle']

# Recurse the source directory, and build the PICU!
subdir('dpn')

