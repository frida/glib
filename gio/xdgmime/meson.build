xdgmime_sources = files(
  'xdgmime.c',
  'xdgmimealias.c',
  'xdgmimecache.c',
  'xdgmimeglob.c',
  'xdgmimeicon.c',
  'xdgmimeint.c',
  'xdgmimemagic.c',
  'xdgmimeparent.c',
)

xdgmime_lib = static_library('xdgmime',
  sources : [xdgmime_sources, glib_built_headers],
  include_directories : [configinc, glibinc],
  pic : true,
  c_args : [ '-DHAVE_CONFIG_H',
             '-DXDG_PREFIX=_gio_xdg' ],
  gnu_symbol_visibility : 'hidden',
)
