project('xmms2-ynca', 'cpp')

boost = dependency('boost', modules : ['system', 'program_options'])
glib = dependency('glib-2.0')
threads = dependency('threads')
xmms2 = dependency('xmms2-client-cpp')
xmms2_glib = dependency('xmms2-client-cpp-glib')

executable('xmms2-ynca', 'xmms2-ynca.cpp', dependencies : [boost, glib, threads, xmms2, xmms2_glib])
