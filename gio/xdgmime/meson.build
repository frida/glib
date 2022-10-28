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

# glib enables various warnings which the xdgmime code wasn’t designed to
# work with
extra_xdgmime_args = cc.get_supported_arguments([
  '-Wno-sign-conversion',
])

xdgmime_lib = static_library('xdgmime',
  sources : [xdgmime_sources, glib_built_headers],
  include_directories : [configinc, glibinc],
  pic : true,
  c_args : [
    '-DHAVE_CONFIG_H',
    '-DXDG_PREFIX=_gio_xdg',
    extra_xdgmime_args,
  ],
  gnu_symbol_visibility : 'hidden',
)
