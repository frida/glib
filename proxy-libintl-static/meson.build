install_headers('libintl.h', subdir : 'glib-2.0')

intl_lib = static_library('intl',
  'libintl.c',
  c_args : ['-DSTUB_ONLY'],
  include_directories : [configinc])

intl_dep = declare_dependency(include_directories : include_directories('.'))
