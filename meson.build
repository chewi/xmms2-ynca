project('xmms2-ynca', 'cpp', default_options : ['cpp_std=gnu++17'])

boost = dependency('boost', modules : ['system', 'program_options'])
glib = dependency('glib-2.0')
threads = dependency('threads')
xmms2 = dependency('xmms2-client-cpp')
xmms2_glib = dependency('xmms2-client-cpp-glib')

fs = import('fs')
executable('xmms2-ynca', 'xmms2-ynca.cpp', dependencies : [boost, glib, threads, xmms2, xmms2_glib], install : true, install_dir : fs.expanduser('~/.config/xmms2/startup.d'))
install_data('ynca.conf', install_dir : fs.expanduser('~/.config/xmms2/clients'))
