# glib enables various warnings which the libcharset code wasn’t designed to
# work with
extra_charset_args = cc.get_supported_arguments([
  '-Wno-sign-conversion',
])

charset_lib = static_library('charset', 'localcharset.c', glib_built_headers,
  include_directories : [configinc, glibinc],
  pic : true,
  c_args : [
    '-DGLIB_CHARSETALIAS_DIR="@0@"'.format(glib_charsetaliasdir),
    extra_charset_args,
  ],
  gnu_symbol_visibility : 'hidden',
)
