charset_lib = static_library('charset', 'localcharset.c', glib_built_headers,
  include_directories : [configinc, glibinc],
  pic : true,
  c_args : [ '-DGLIB_CHARSETALIAS_DIR="@0@"'.format(glib_charsetaliasdir) ],
  gnu_symbol_visibility : 'hidden',
)
