gio_c_args = [
  '-DG_LOG_DOMAIN="GLib-GIO"',
  '-DGIO_LAUNCH_DESKTOP="@0@"'.format(glib_prefix / multiarch_libexecdir / 'gio-launch-desktop'),
  '-DGIO_MODULE_DIR="@0@"'.format(glib_giomodulesdir),
  '-DLOCALSTATEDIR="@0@"'.format(glib_localstatedir),
]

gio_c_args_internal = [
  '-DGIO_COMPILATION',
]

# Install empty glib_giomodulesdir
install_emptydir(glib_giomodulesdir)

gio_includedir = glib_includedir / 'gio'

gnetworking_h_conf = configuration_data()

gnetworking_h_wspiapi_include = ''
gnetworking_h_nameser_compat_include = ''

if host_system == 'windows'
  # <wspiapi.h> in the Windows SDK and in mingw-w64 has wrappers for
  # inline workarounds for getaddrinfo, getnameinfo and freeaddrinfo if
  # they aren't present at run-time (on Windows 2000).
  gnetworking_h_wspiapi_include = '#include <wspiapi.h>'
elif host_system not in ['android', 'none']
  # Don't check for C_IN on Android since it does not define it in public
  # headers, we define it ourselves wherever necessary
  if not cc.compiles('''#include <sys/types.h>
                        #include <arpa/nameser.h>
                        int qclass = C_IN;''',
                     name : 'C_IN in public headers (no arpa/nameser_compat.h needed)')
    if cc.compiles('''#include <sys/types.h>
                      #include <arpa/nameser.h>
                      #include <arpa/nameser_compat.h>
                      int qclass = C_IN;''',
                   name : 'arpa/nameser_compat.h needed for C_IN')
      gnetworking_h_nameser_compat_include = '#include <arpa/nameser_compat.h>'
    else
      error('Could not find required includes for ARPA C_IN')
    endif
  endif
endif

network_libs = [ ]
network_args = [ ]
if host_system not in ['windows', 'none']
  # res_query()
  res_query_test = '''#include <resolv.h>
                      int main (int argc, char ** argv) {
                        return res_query("test", 0, 0, (void *)0, 0);
                      }'''
  res_query_test_full = '''#include <sys/types.h>
                           #include <netinet/in.h>
                           #include <arpa/nameser.h>
                        ''' + res_query_test
  if not cc.links(res_query_test_full, name : 'res_query()')
    if cc.links(res_query_test_full, args : '-lresolv', name : 'res_query() in -lresolv')
      network_libs += [ cc.find_library('resolv') ]
      network_args += [ '-lresolv' ]
    elif cc.links(res_query_test, args : '-lbind', name : 'res_query() in -lbind')
      network_libs += [ cc.find_library('bind') ]
      network_args += [ '-lbind' ]
    elif cc.links(res_query_test, args : '-lsocket', name : 'res_query() in -lsocket')
      network_libs += [ cc.find_library('socket') ]
      network_args += [ '-lsocket' ]
    else
      error('Could not find res_query()')
    endif
  endif

  # socket()
  socket_test = '''#include <sys/types.h>
                   #include <sys/socket.h>
                   int main (int argc, char ** argv) {
                     return socket(1, 2, 3);
                   }'''
  if not cc.links(socket_test, name : 'socket()')
    if cc.links(socket_test, args : '-lsocket', name : 'socket() in -lsocket')
      network_libs += [ cc.find_library('socket') ]
      network_args += [ '-lsocket' ]
    else
      error('Could not find socket()')
    endif
  endif

  # dn_comp()
  if cc.links('''#include <resolv.h>
                 int main (int argc, char ** argv) {
                   return dn_comp(NULL, NULL, 0, NULL, NULL) == -1;
                 } ''', args : network_args, name : 'dn_comp()')
    glib_conf.set('HAVE_DN_COMP', 1)
  endif

  # res_nclose()
  if cc.links('''#include <sys/types.h>
                 #include <netinet/in.h>
                 #include <arpa/nameser.h>
                 #include <resolv.h>
                 int main (int argc, char ** argv) {
                   struct __res_state res;
                   res_nclose(&res);
                   return 0;
                 }''', args : network_args, name : 'res_nclose()')
    glib_conf.set('HAVE_RES_NCLOSE', 1)
  endif

  # res_ndestroy()
  if cc.links('''#include <sys/types.h>
                 #include <netinet/in.h>
                 #include <arpa/nameser.h>
                 #include <resolv.h>
                 int main (int argc, char ** argv) {
                   struct __res_state res;
                   res_ndestroy(&res);
                   return 0;
                 }''', args : network_args, name : 'res_ndestroy()')
    glib_conf.set('HAVE_RES_NDESTROY', 1)
  endif

  # res_ninit()
  if cc.links('''#include <sys/types.h>
                 #include <netinet/in.h>
                 #include <arpa/nameser.h>
                 #include <resolv.h>
                 int main (int argc, char ** argv) {
                   struct __res_state res;
                   return res_ninit(&res);
                 }''', args : network_args, name : 'res_ninit()')
    glib_conf.set('HAVE_RES_NINIT', 1)
  endif

  # res_nquery()
  if cc.links('''#include <sys/types.h>
                 #include <netinet/in.h>
                 #include <arpa/nameser.h>
                 #include <resolv.h>
                 int main (int argc, char ** argv) {
                   struct __res_state res;
                   return res_nquery(&res, "test", 0, 0, (void *)0, 0);
                 }''', args : network_args, name : 'res_nquery()')
    glib_conf.set('HAVE_RES_NQUERY', 1)
  endif

  if cc.has_type('struct ip_mreqn', prefix : '#include <netinet/in.h>')
    glib_conf.set('HAVE_IP_MREQN', 1)
  endif

  if cc.compiles('''#include <sys/ioctl.h>
                    #include <net/if.h>
                    int main (int argc, char ** argv) {
                      struct ifreq ifr;
                      ioctl(0, SIOCGIFADDR, &ifr);
                      return 0;
                    }''',
                 name : 'ioctl with request SIOCGIFADDR')
    glib_conf.set('HAVE_SIOCGIFADDR', '/**/')
  endif

endif

if host_system == 'android'
  # struct ip_mreq_source definition is broken on Android NDK <= r16
  # See https://bugzilla.gnome.org/show_bug.cgi?id=740791
  if not cc.compiles('''#include <netinet/in.h>
                        int main(int argc, char ** argv) {
                          struct ip_mreq_source mc_req_src;
                          mc_req_src.imr_interface.s_addr = 0;
                          return 0;
                        }''',
                        name : 'ip_mreq_source.imr_interface has s_addr member')
    glib_conf.set('BROKEN_IP_MREQ_SOURCE_STRUCT', 1)
  endif
endif

gnetworking_h_conf.set('WSPIAPI_INCLUDE', gnetworking_h_wspiapi_include)
gnetworking_h_conf.set('NAMESER_COMPAT_INCLUDE', gnetworking_h_nameser_compat_include)

gnetworking_h = configure_file(input : 'gnetworking.h.in',
                               output : 'gnetworking.h',
                               install_dir : gio_includedir,
                               configuration : gnetworking_h_conf)

gdbus_headers = files(
  'gdbusauthobserver.h',
  'gcredentials.h',
  'gdbusutils.h',
  'gdbuserror.h',
  'gdbusaddress.h',
  'gdbusconnection.h',
  'gdbusmessage.h',
  'gdbusnameowning.h',
  'gdbusnamewatching.h',
  'gdbusproxy.h',
  'gdbusintrospection.h',
  'gdbusmethodinvocation.h',
  'gdbusserver.h',
  'gdbusinterface.h',
  'gdbusinterfaceskeleton.h',
  'gdbusobject.h',
  'gdbusobjectskeleton.h',
  'gdbusobjectproxy.h',
  'gdbusobjectmanager.h',
  'gdbusobjectmanagerclient.h',
  'gdbusobjectmanagerserver.h',
  'gtestdbus.h',
)

gdbus_sources = files(
  'gdbusutils.c',
  'gdbusaddress.c',
  'gdbusauthobserver.c',
  'gdbusauth.c',
  'gdbusauthmechanism.c',
  'gdbusauthmechanismanon.c',
  'gdbusauthmechanismexternal.c',
  'gdbusauthmechanismsha1.c',
  'gdbuserror.c',
  'gdbusconnection.c',
  'gdbusmessage.c',
  'gdbusnameowning.c',
  'gdbusnamewatching.c',
  'gdbusproxy.c',
  'gdbusprivate.c',
  'gdbusintrospection.c',
  'gdbusmethodinvocation.c',
  'gdbusserver.c',
  'gdbusinterface.c',
  'gdbusinterfaceskeleton.c',
  'gdbusobject.c',
  'gdbusobjectskeleton.c',
  'gdbusobjectproxy.c',
  'gdbusobjectmanager.c',
  'gdbusobjectmanagerclient.c',
  'gdbusobjectmanagerserver.c',
  'gtestdbus.c',
)

# Generate gdbus-codegen
subdir('gdbus-2.0/codegen')

# Generate xdp-dbus.{c,h}
xdp_dbus_generated = custom_target('xdp-dbus',
    input : ['org.freedesktop.portal.Documents.xml',
             'org.freedesktop.portal.OpenURI.xml',
             'org.freedesktop.portal.ProxyResolver.xml',
             'org.freedesktop.portal.Trash.xml'],
    output : ['xdp-dbus.h', 'xdp-dbus.c'],
    depend_files : gdbus_codegen_built_files,
    command : [python, gdbus_codegen,
               '--interface-prefix', 'org.freedesktop.portal.',
               '--output-directory', '@OUTDIR@',
               '--generate-c-code', 'xdp-dbus',
               '--c-namespace', 'GXdp',
               '@INPUT@'])

# Generate gdbus-generated.{c,h}
gdbus_daemon_generated = custom_target('gdbus-daemon-generated',
    input : ['dbus-daemon.xml'],
    output : ['gdbus-daemon-generated.h', 'gdbus-daemon-generated.c'],
    depend_files : gdbus_codegen_built_files,
    command : [python, gdbus_codegen,
               '--interface-prefix', 'org.',
               '--output-directory', '@OUTDIR@',
               '--generate-c-code', 'gdbus-daemon-generated',
               '--c-namespace', '_G', '@INPUT@'])

settings_headers = files(
  'gsettingsbackend.h',
  'gsettingsschema.h',
  'gsettings.h',
)

settings_sources = files(
  'gdelayedsettingsbackend.c',
  'gkeyfilesettingsbackend.c',
  'gmemorysettingsbackend.c',
  'gnullsettingsbackend.c',
  'gsettingsbackend.c',
  'gsettingsschema.c',
  'gsettings-mapping.c',
  'gsettings.c',
)

if host_system == 'windows'
  settings_sources += files('gregistrysettingsbackend.c')
endif

application_headers = files(
  'gapplication.h',
  'gapplicationcommandline.h',

  'gactiongroup.h',
  'gactionmap.h',
  'gsimpleactiongroup.h',
  'gremoteactiongroup.h',
  'gactiongroupexporter.h',
  'gdbusactiongroup.h',
  'gaction.h',
  'gpropertyaction.h',
  'gsimpleaction.h',

  'gmenumodel.h',
  'gmenu.h',
  'gmenuexporter.h',
  'gdbusmenumodel.h',
  'gnotification.h',
)

application_sources = files(
  'gapplication.c',
  'gapplicationcommandline.c',
  'gapplicationimpl-dbus.c',

  'gactiongroup.c',
  'gactionmap.c',
  'gsimpleactiongroup.c',
  'gremoteactiongroup.c',
  'gactiongroupexporter.c',
  'gdbusactiongroup.c',
  'gaction.c',
  'gpropertyaction.c',
  'gsimpleaction.c',

  'gmenumodel.c',
  'gmenu.c',
  'gmenuexporter.c',
  'gdbusmenumodel.c',
  'gnotification.c',
  'gnotificationbackend.c',
)

local_sources = files(
  'ghttpproxy.c',
  'glocalfile.c',
  'glocalfileenumerator.c',
  'glocalfileinfo.c',
  'glocalfileinputstream.c',
  'glocalfilemonitor.c',
  'glocalfileoutputstream.c',
  'glocalfileiostream.c',
  'glocalvfs.c',
  'gsocks4proxy.c',
  'gsocks4aproxy.c',
  'gsocks5proxy.c',
  'thumbnail-verify.c',
)

platform_deps = []
internal_deps = []
appinfo_sources = []
contenttype_sources = []
portal_sources = []
unix_sources = []
win32_sources = []

# This is also used by tests/gdbus-daemon, so use files() to include the path
gdbus_daemon_sources = [
  files('gdbusdaemon.c'),
  gdbus_daemon_generated,
]

if host_system not in ['windows', 'none']
  unix_sources = files(
    'gfiledescriptorbased.c',
    'giounix-private.c',
    'gunixfdmessage.c',
    'gunixmount.c',
    'gunixmounts.c',
    'gunixvolume.c',
    'gunixvolumemonitor.c',
    'gunixinputstream.c',
    'gunixoutputstream.c',
    'gfdonotificationbackend.c',
    'ggtknotificationbackend.c',
  )

  portal_sources = [files(
      'gdocumentportal.c',
      'gopenuriportal.c',
      'gmemorymonitorportal.c',
      'gnetworkmonitorportal.c',
      'gpowerprofilemonitorportal.c',
      'gproxyresolverportal.c',
      'gtrashportal.c',
      'gportalsupport.c',
      'gportalnotificationbackend.c',
      'gsandbox.c',
    ),
    xdp_dbus_generated
  ]

  gio_unix_include_headers = files(
    'gfiledescriptorbased.h',
    'gunixmounts.h',
    'gunixfdmessage.h',
    'gunixinputstream.h',
    'gunixoutputstream.h',
  )

  if glib_have_cocoa
    settings_sources += files('gnextstepsettingsbackend.m')
    contenttype_sources += files('gosxcontenttype.m')
    appinfo_sources += files('gosxappinfo.m')
    framework_dep = dependency('appleframeworks', modules : ['Foundation', 'CoreFoundation', 'AppKit'])
    platform_deps += [framework_dep]
    if glib_have_os_x_9_or_later
      unix_sources += files('gcocoanotificationbackend.m')
    endif
    application_headers += files('gosxappinfo.h')
  else
    contenttype_sources += files('gcontenttype.c')
    appinfo_sources += files('gdesktopappinfo.c')
    gio_unix_include_headers += files('gdesktopappinfo.h')
    launch_desktop_sources = files('gio-launch-desktop.c')

    if host_system == 'linux'
      launch_desktop_sources += files('../glib/gjournal-private.c')
    endif

    gio_launch_desktop = executable('gio-launch-desktop', launch_desktop_sources,
      include_directories : glibinc,
      install : true,
      install_dir : multiarch_libexecdir,
      install_tag : 'bin',
      c_args : gio_c_args,
      # intl.lib is not compatible with SAFESEH
      link_args : noseh_link_args)
  endif

  subdir('xdgmime')
  internal_deps += [xdgmime_lib]

  install_headers(gio_unix_include_headers, subdir : 'gio-unix-2.0/gio')

  if glib_conf.has('HAVE_NETLINK')
    unix_sources += files(
      'gnetworkmonitornetlink.c',
      'gnetworkmonitornm.c',
    )
  endif
elif host_system == 'windows'
  appinfo_sources += files('gwin32appinfo.c')
  contenttype_sources += files('gcontenttype-win32.c')
  platform_deps += [cc.find_library('shlwapi'),
                    cc.find_library('dnsapi'),
                    iphlpapi_dep,
                    winsock2]
  platform_deps += uwp_gio_deps

  win32_sources += files(
    'gmemorymonitorwin32.c',
    'gwin32registrykey.c',
    'gwin32mount.c',
    'gwin32volumemonitor.c',
    'gwin32inputstream.c',
    'gwin32outputstream.c',
    'gwin32file-sync-stream.c',
    'gwin32packageparser.c',
    'gwin32networkmonitor.c',
    'gwin32networkmonitor.h',
    'gwin32notificationbackend.c',
    'gwin32sid.c',
    'gwin32sid.h',
  )

  if get_option('default_library') != 'static'
    gio_win_rc = configure_file(
      input: 'gio.rc.in',
      output: 'gio.rc',
      configuration: glibconfig_conf,
    )
    gio_win_res = windows.compile_resources(gio_win_rc)
    win32_sources += [gio_win_res]
  endif

  gio_win32_include_headers = files(
    'gwin32inputstream.h',
    'gwin32outputstream.h',
  )
  install_headers(gio_win32_include_headers, subdir : 'gio-win32-2.0/gio')
endif

gio_sources = files(
  'gio-init.c',
  'gio-fork.c',
  'gappinfo.c',
  'gasynchelper.c',
  'gasyncinitable.c',
  'gasyncresult.c',
  'gbufferedinputstream.c',
  'gbufferedoutputstream.c',
  'gbytesicon.c',
  'gcancellable.c',
  'gcharsetconverter.c',
  'gcontextspecificgroup.c',
  'gconverter.c',
  'gconverterinputstream.c',
  'gconverteroutputstream.c',
  'gcredentials.c',
  'gdatagrambased.c',
  'gdatainputstream.c',
  'gdataoutputstream.c',
  'gdebugcontroller.c',
  'gdebugcontrollerdbus.c',
  'gdrive.c',
  'gdummyfile.c',
  'gdummyproxyresolver.c',
  'gdummytlsbackend.c',
  'gemblem.c',
  'gemblemedicon.c',
  'gfilterinputstream.c',
  'gfilteroutputstream.c',
  'gicon.c',
  'ginetaddressmask.c',
  'ginitable.c',
  'ginputstream.c',
  'gioerror.c',
  'giomodule.c',
  'giomodule-priv.c',
  'gioscheduler.c',
  'giostream.c',
  'gloadableicon.c',
  'gmarshal-internal.c',
  'gmount.c',
  'gmemorymonitor.c',
  'gmemorymonitordbus.c',
  'gmemoryinputstream.c',
  'gmemoryoutputstream.c',
  'gmountoperation.c',
  'gnativevolumemonitor.c',
  'gnetworking.c',
  'goutputstream.c',
  'gpermission.c',
  'gpollableinputstream.c',
  'gpollableoutputstream.c',
  'gpollableutils.c',
  'gpollfilemonitor.c',
  'gpowerprofilemonitor.c',
  'gpowerprofilemonitordbus.c',
  'gproxy.c',
  'gproxyaddress.c',
  'gproxyaddressenumerator.c',
  'gproxyresolver.c',
  'gresource.c',
  'gresourcefile.c',
  'gseekable.c',
  'gsimpleasyncresult.c',
  'gsimpleiostream.c',
  'gsimplepermission.c',
  'gsimpleproxyresolver.c',
  'gsocketconnectable.c',
  'gsocketconnection.c',
  'gsocketcontrolmessage.c',
  'gsocketinputstream.c',
  'gsocketlistener.c',
  'gsocketoutputstream.c',
  'gsocketservice.c',
  'gsrvtarget.c',
  'gtask.c',
  'gtcpconnection.c',
  'gtcpwrapperconnection.c',
  'gthemedicon.c',
  'gthreadedsocketservice.c',
  'gthreadedresolver.h',
  'gtlsbackend.c',
  'gtlscertificate.c',
  'gtlsclientconnection.c',
  'gtlsconnection.c',
  'gtlsdatabase.c',
  'gtlsfiledatabase.c',
  'gtlsinteraction.c',
  'gtlspassword.c',
  'gtlsserverconnection.c',
  'gdtlsconnection.c',
  'gdtlsclientconnection.c',
  'gdtlsserverconnection.c',
  'gunionvolumemonitor.c',
  'gunixconnection.c',
  'gunixfdlist.c',
  'gunixcredentialsmessage.c',
  'gvfs.c',
  'gvolume.c',
  'gvolumemonitor.c',
  'gzlibcompressor.c',
  'gzlibdecompressor.c',
  'glistmodel.c',
  'gliststore.c',
)

if host_system != 'none'
  gio_sources += files(
    'gfile.c',
    'gfileattribute.c',
    'gfileenumerator.c',
    'gfileicon.c',
    'gfileinfo.c',
    'gfileinputstream.c',
    'gfileiostream.c',
    'gfilemonitor.c',
    'gfilenamecompleter.c',
    'gfileoutputstream.c',
    'ginetaddress.c',
    'ginetsocketaddress.c',
    'gnativesocketaddress.c',
    'gnetworkaddress.c',
    'gnetworkmonitor.c',
    'gnetworkmonitorbase.c',
    'gnetworkservice.c',
    'gresolver.c',
    'gsocket.c',
    'gsocketaddress.c',
    'gsocketaddressenumerator.c',
    'gsocketclient.c',
    'gsubprocess.c',
    'gsubprocesslauncher.c',
    'gthreadedresolver.c',
    'gunixsocketaddress.c',
  )
endif

if glib_build_shared
  gio_sources += files ('../glib/gtrace.c')
endif

gio_sources += appinfo_sources
gio_sources += contenttype_sources
if host_system != 'none'
  gio_sources += gdbus_daemon_sources
  gio_sources += unix_sources
  gio_sources += win32_sources
  gio_sources += application_sources
endif
gio_sources += settings_sources
if host_system != 'none'
  gio_sources += gdbus_sources
  gio_sources += portal_sources
  gio_sources += local_sources
endif

MISSING_STUFF = '''
# This is read by gobject-introspection/misc/ and gtk-doc
gio-public-headers.txt: Makefile
  '$(AM_V_GEN) echo $(gioinclude_HEADERS) $(giowin32include_HEADERS) $(giounixinclude_HEADERS) > $@.tmp && mv $@.tmp $@

gio.def: libgio-2.0.la
  '$(AM_V_GEN) dumpbin.exe -exports .libs/libgio-2.0-0.dll | awk 'BEGIN { print "EXPORTS" } / +[[:digit:]]+ +[[:xdigit:]]+ +[[:xdigit:]]+/{ print $$4 }' > gio.def.tmp && mv gio.def.tmp gio.def

gio-2.0.lib: libgio-2.0.la gio.def
  '$(AM_V_GEN) lib.exe -machine:@LIB_EXE_MACHINE_FLAG@ -name:libgio-2.0-$(LT_CURRENT_MINUS_AGE).dll -def:$(builddir)/gio.def -out:$@
'''

gio_headers = files(
  'gappinfo.h',
  'gasyncinitable.h',
  'gasyncresult.h',
  'gbufferedinputstream.h',
  'gbufferedoutputstream.h',
  'gbytesicon.h',
  'gcancellable.h',
  'gcontenttype.h',
  'gcharsetconverter.h',
  'gconverter.h',
  'gconverterinputstream.h',
  'gconverteroutputstream.h',
  'gdatagrambased.h',
  'gdatainputstream.h',
  'gdataoutputstream.h',
  'gdebugcontroller.h',
  'gdebugcontrollerdbus.h',
  'gdrive.h',
  'gemblem.h',
  'gemblemedicon.h',
  'gfile.h',
  'gfileattribute.h',
  'gfileenumerator.h',
  'gfileicon.h',
  'gfileinfo.h',
  'gfileinputstream.h',
  'gfilemonitor.h',
  'gfilenamecompleter.h',
  'gfileoutputstream.h',
  'gfileiostream.h',
  'gfilterinputstream.h',
  'gfilteroutputstream.h',
  'gicon.h',
  'ginetaddress.h',
  'ginetaddressmask.h',
  'ginetsocketaddress.h',
  'ginitable.h',
  'ginputstream.h',
  'gio.h',
  'gio-autocleanups.h',
  'gioenums.h',
  'gioerror.h',
  'giomodule.h',
  'gioscheduler.h',
  'giostream.h',
  'giotypes.h',
  'gloadableicon.h',
  'gmount.h',
  'gmemoryinputstream.h',
  'gmemorymonitor.h',
  'gmemoryoutputstream.h',
  'gmountoperation.h',
  'gnativesocketaddress.h',
  'gnativevolumemonitor.h',
  'gnetworkaddress.h',
  'gnetworkmonitor.h',
  'gnetworkservice.h',
  'goutputstream.h',
  'gpermission.h',
  'gpollableinputstream.h',
  'gpollableoutputstream.h',
  'gpollableutils.h',
  'gpowerprofilemonitor.h',
  'gproxy.h',
  'gproxyaddress.h',
  'gproxyaddressenumerator.h',
  'gproxyresolver.h',
  'gresolver.h',
  'gresource.h',
  'gseekable.h',
  'gsimpleasyncresult.h',
  'gsimpleiostream.h',
  'gsimplepermission.h',
  'gsimpleproxyresolver.h',
  'gsocket.h',
  'gsocketaddress.h',
  'gsocketaddressenumerator.h',
  'gsocketclient.h',
  'gsocketconnectable.h',
  'gsocketconnection.h',
  'gsocketcontrolmessage.h',
  'gsocketlistener.h',
  'gsocketservice.h',
  'gsrvtarget.h',
  'gsubprocess.h',
  'gsubprocesslauncher.h',
  'gtask.h',
  'gtcpconnection.h',
  'gtcpwrapperconnection.h',
  'gthemedicon.h',
  'gthreadedsocketservice.h',
  'gtlsbackend.h',
  'gtlscertificate.h',
  'gtlsclientconnection.h',
  'gtlsconnection.h',
  'gtlsdatabase.h',
  'gtlsfiledatabase.h',
  'gtlsinteraction.h',
  'gtlspassword.h',
  'gtlsserverconnection.h',
  'gdtlsconnection.h',
  'gdtlsclientconnection.h',
  'gdtlsserverconnection.h',
  'gunixconnection.h',
  'gunixcredentialsmessage.h',
  'gunixfdlist.h',
  'gunixsocketaddress.h',
  'gvfs.h',
  'gvolume.h',
  'gvolumemonitor.h',
  'gzlibcompressor.h',
  'gzlibdecompressor.h',
  'glistmodel.h',
  'gliststore.h',
)

gvisibility_h = custom_target(
  output: 'gio-visibility.h',
  command: [gen_visibility_macros, meson.project_version(), 'visibility-macros', 'GIO', '@OUTPUT@'],
  install: true,
  install_dir: gio_includedir,
  # FIXME: Not needed with Meson >= 0.64.0
  install_tag: 'devel',
)
gio_sources += gvisibility_h

gio_headers += application_headers
gio_headers += settings_headers
gio_headers += gdbus_headers
install_headers(gio_headers, install_dir : gio_includedir)

# We can't use gnome.mkenums() because the GNOME module looks for glib-mkenums
# in PATH, which means you can't bootstrap glib with its own glib-mkenums.
gioenumtypes_h = custom_target('gioenumtypes_h',
  output : 'gioenumtypes.h',
  capture : true,
  input : gio_headers,
  install : true,
  install_dir : gio_includedir,
  # FIXME: Not needed with Meson >= 0.64.0
  install_tag: 'devel',
  command : [python, glib_mkenums,
             '--template', files('gioenumtypes.h.template'),
             '@INPUT@', gnetworking_h])

gioenumtypes_c = custom_target('gioenumtypes_c',
  output : 'gioenumtypes.c',
  capture : true,
  input : gio_headers,
  depends : [gioenumtypes_h],
  command : [python, glib_mkenums,
             '--template', files('gioenumtypes.c.template'),
             '@INPUT@', gnetworking_h])

gioenumtypes_dep = declare_dependency(sources : [gioenumtypes_h, glib_enumtypes_h, glib_built_headers, gvisibility_h],
  dependencies : [libgmodule_dep])

# inotify
if host_system in ['linux', 'android']
  subdir('inotify')
  internal_deps += [ inotify_lib ]
endif

# kevent
if have_func_kqueue and have_func_kevent
  subdir('kqueue')
  internal_deps += [ kqueue_lib ]
endif

if host_system == 'windows'
  subdir('win32')
  internal_deps += [ giowin32_lib ]
endif

if have_bash
  bash_comp_inst_dir = ''
  if bash_comp_dep.found()
    bash_comp_dir_override = bash_comp_dep.version().version_compare('>= 2.10') ? ['datadir', get_option('datadir')] : ['prefix', get_option('prefix')]
    bash_comp_inst_dir = bash_comp_dep.get_variable('completionsdir', pkgconfig_define: bash_comp_dir_override)
  endif

  if bash_comp_inst_dir == ''
    message('Found bash-completion but the .pc file did not set \'completionsdir\', fallback to a predefined path')
    bash_comp_inst_dir = join_paths(get_option('datadir'), 'bash-completion/completions')
  endif

  install_data([
      'completion/gapplication',
      'completion/gdbus',
      'completion/gio',
      'completion/gsettings',
      'completion/gresource'
    ],
    install_dir: bash_comp_inst_dir,
    install_tag: 'bin',
  )
endif

if enable_dtrace
  gio_dtrace_obj = dtrace_obj_gen.process('gio_probes.d')
  gio_dtrace_hdr = dtrace_hdr_gen.process('gio_probes.d')
else
  gio_dtrace_obj = []
  gio_dtrace_hdr = []
endif

libgio = library('gio-2.0',
  gioenumtypes_h, gioenumtypes_c, gnetworking_h, gio_sources,
  gio_dtrace_hdr, gio_dtrace_obj,
  version : library_version,
  soversion : soversion,
  darwin_versions : darwin_versions,
  install : true,
  include_directories : [configinc, gioinc],
  #  '$(gio_win32_res_ldflag)',
  link_with: internal_deps,
  dependencies : [libz_dep, libdl_dep, libmount_dep, libglib_dep,
                  libgobject_dep, libgmodule_dep, selinux_dep, xattr_dep,
                  platform_deps, network_libs, libsysprof_capture_dep,
                  gioenumtypes_dep, gvdb_dep],
  c_args : [gio_c_args, gio_c_args_internal],
  objc_args : [gio_c_args, gio_c_args_internal],
  gnu_symbol_visibility : 'hidden',
  # intl.lib is not compatible with SAFESEH
  link_args : [noseh_link_args, glib_link_flags],
)

if get_option('gio_module_dir') != ''
  pkgconfig_giomodulesdir = join_paths('${prefix}', get_option('gio_module_dir'))
else
  pkgconfig_giomodulesdir = join_paths('${libdir}', 'gio', 'modules')
endif

schemas_subdir = join_paths('glib-2.0', 'schemas')

libgio_dep = declare_dependency(link_with : libgio,
  dependencies : [libgmodule_dep, libgobject_dep, gioenumtypes_dep],
  sources: gvisibility_h,
  include_directories : [gioinc],
  variables : [
    'schemasdir=' + join_paths(glib_datadir, schemas_subdir),
    'giomoduledir=' + glib_giomodulesdir,
  ],
)

pkg.generate(libgio,
  requires : ['glib-2.0', 'gobject-2.0'],
  variables : ['datadir=' + join_paths('${prefix}', get_option('datadir')),
               'schemasdir=' + join_paths('${datadir}', schemas_subdir),
               'bindir=' + join_paths('${prefix}', get_option('bindir')),
               'giomoduledir=' + pkgconfig_giomodulesdir,
               'gio=' + join_paths('${bindir}', 'gio'),
               'gio_querymodules=@0@'.format(pkgconfig_multiarch_bindir / 'gio-querymodules'),
               'glib_compile_schemas=@0@'.format(pkgconfig_multiarch_bindir / 'glib-compile-schemas'),
               'glib_compile_resources=' + join_paths('${bindir}', 'glib-compile-resources'),
               'gdbus=' + join_paths('${bindir}', 'gdbus'),
               'gdbus_codegen=' + join_paths('${bindir}', 'gdbus-codegen'),
               'gresource=' + join_paths('${bindir}', 'gresource'),
               'gsettings=' + join_paths('${bindir}', 'gsettings')],
  version : glib_version,
  install_dir : glib_pkgconfigreldir,
  filebase : 'gio-2.0',
  name : 'GIO',
  description : 'glib I/O library',
)
meson.override_dependency('gio-2.0', libgio_dep)

if host_system == 'windows'
  pkg.generate(requires : ['gobject-2.0', 'gmodule-no-export-2.0', 'gio-2.0'],
    subdirs : ['gio-win32-2.0'],
    version : glib_version,
    install_dir : glib_pkgconfigreldir,
    filebase : 'gio-windows-2.0',
    name : 'GIO Windows specific APIs',
    description : 'Windows specific headers for glib I/O library',
  )
  meson.override_dependency('gio-windows-2.0', libgio_dep)
else
  pkg.generate(requires : ['gobject-2.0', 'gio-2.0'],
    subdirs : ['gio-unix-2.0'],
    version : glib_version,
    install_dir : glib_pkgconfigreldir,
    filebase : 'gio-unix-2.0',
    name : 'GIO unix specific APIs',
    description : 'unix specific headers for glib I/O library',
  )
  meson.override_dependency('gio-unix-2.0', libgio_dep)
endif

if host_system == 'windows'
  # Hack till https://github.com/mesonbuild/meson/issues/2324 is fixed
  libgiounix_dep = dependency('', required : false)
  libgiowin32_dep = libgio_dep
else
  libgiowin32_dep = dependency('', required : false)
  libgiounix_dep = libgio_dep
endif

# Dependencies used by executables below
have_libelf = false
libelf = dependency('libelf', version : '>= 0.8.12', required : get_option ('libelf'))
if libelf.found()
  have_libelf = true
else
  # This fallback is necessary on *BSD. elfutils isn't the only libelf
  # implementation, and *BSD usually includes their own libelf as a system
  # library which doesn't have a corresponding .pc file.
  libelf = cc.find_library('elf', required : get_option ('libelf'))
  have_libelf = libelf.found()
  have_libelf = have_libelf and cc.has_function('elf_begin', dependencies : libelf)
  have_libelf = have_libelf and cc.has_function('elf_getshdrstrndx', dependencies : libelf)
  have_libelf = have_libelf and cc.has_function('elf_getshdrnum', dependencies : libelf)
  have_libelf = have_libelf and cc.has_header('libelf.h')
endif

if have_libelf
  glib_conf.set('HAVE_LIBELF', 1)
else
  libelf = []
endif

gconstructor_as_data_h = custom_target('gconstructor_as_data.h',
    input : ['data-to-c.py', files('../glib/gconstructor.h')],
    output : ['gconstructor_as_data.h'],
    command : [python, '@INPUT0@', '@INPUT1@', 'gconstructor_code', '@OUTPUT@'])

# Several installed executables
gio_tool_sources = [
  'gio-tool.c',
  'gio-tool.h',
  'gio-tool-cat.c',
  'gio-tool-copy.c',
  'gio-tool-info.c',
  'gio-tool-launch.c',
  'gio-tool-list.c',
  'gio-tool-mime.c',
  'gio-tool-mkdir.c',
  'gio-tool-monitor.c',
  'gio-tool-mount.c',
  'gio-tool-move.c',
  'gio-tool-open.c',
  'gio-tool-rename.c',
  'gio-tool-remove.c',
  'gio-tool-save.c',
  'gio-tool-set.c',
  'gio-tool-trash.c',
  'gio-tool-tree.c',
]

if host_system != 'none'

executable('gio', gio_tool_sources,
  install : true,
  install_tag : 'bin',
  c_args : gio_c_args,
  # intl.lib is not compatible with SAFESEH
  link_args : noseh_link_args,
  dependencies : [libgio_dep, libgobject_dep, libgmodule_dep, libglib_dep])

executable('gresource', 'gresource-tool.c',
  install : true,
  install_tag : 'bin',
  # intl.lib is not compatible with SAFESEH
  link_args : noseh_link_args,
  dependencies : [libelf, libgio_dep, libgobject_dep, libgmodule_dep, libglib_dep])

gio_querymodules = executable('gio-querymodules', 'gio-querymodules.c', 'giomodule-priv.c',
  install : true,
  install_dir : multiarch_bindir,
  install_tag : 'bin',
  c_args : gio_c_args,
  # intl.lib is not compatible with SAFESEH
  link_args : noseh_link_args,
  dependencies : [libgio_dep, libgobject_dep, libgmodule_dep, libglib_dep])

glib_compile_schemas = executable('glib-compile-schemas',
  ['glib-compile-schemas.c'],
  install : true,
  install_dir : multiarch_bindir,
  install_tag : 'bin',
  # intl.lib is not compatible with SAFESEH
  link_args : noseh_link_args,
  dependencies : [libgio_dep, libgobject_dep, libgmodule_dep, libglib_dep, gvdb_dep])

glib_compile_resources = executable('glib-compile-resources',
  [gconstructor_as_data_h, 'glib-compile-resources.c'],
  install : true,
  install_tag : 'bin-devel',
  c_args : gio_c_args,
  # intl.lib is not compatible with SAFESEH
  link_args : noseh_link_args,
  dependencies : [libgio_dep, libgobject_dep, libgmodule_dep, libglib_dep, gvdb_dep])

# Cannot override those programs in cross compilation case because they are
# native executables that cannot be run on the build machine.
# See https://gitlab.gnome.org/GNOME/glib/issues/1859.
if meson.can_run_host_binaries()
  meson.override_find_program('glib-compile-schemas', glib_compile_schemas)
  meson.override_find_program('glib-compile-resources', glib_compile_resources)
  meson.override_find_program('gio-querymodules', gio_querymodules)
endif

executable('gsettings', 'gsettings-tool.c',
  install : true,
  install_tag : 'bin',
  c_args : gio_c_args,
  # intl.lib is not compatible with SAFESEH
  link_args : noseh_link_args,
  dependencies : [libgio_dep, libgobject_dep, libgmodule_dep, libglib_dep])
install_data('gschema.dtd',
  install_dir : get_option('datadir') / schemas_subdir,
  install_tag : 'devel',
)

endif

install_data(['gschema.loc', 'gschema.its'],
  install_dir : get_option('datadir') / 'gettext' / 'its',
  install_tag : 'devel',
)

if host_system != 'none'

executable('gdbus', 'gdbus-tool.c',
  install : true,
  install_tag : 'bin',
  c_args : gio_c_args,
  # intl.lib is not compatible with SAFESEH
  link_args : noseh_link_args,
  dependencies : [libgio_dep, libgobject_dep, libgmodule_dep, libglib_dep])

if host_system != 'windows' and not glib_have_cocoa
  executable('gapplication', 'gapplication-tool.c',
    install : true,
    install_tag : 'bin',
    c_args : gio_c_args,
    # intl.lib is not compatible with SAFESEH
    link_args : noseh_link_args,
    dependencies : [libgio_dep, libgobject_dep, libgmodule_dep, libglib_dep])
endif

endif

if enable_systemtap
  gio_stp = configure_file(input : 'gio.stp.in',
    output : '@0@.stp'.format(libgio.full_path().split('/').get(-1)),
    configuration : stp_cdata,
    install_dir : tapset_install_dir,
    install_tag : 'systemtap',
  )
endif

if multiarch_bindir != get_option('bindir')
  foreach exe : ['gio-querymodules', 'glib-compile-schemas']
    if meson.version().version_compare('>=0.61.0')
      install_symlink(
        exe,
        install_dir : get_option('bindir'),
        pointing_to : get_option('prefix') / multiarch_bindir / exe,
      )
    else
      warning(
        'Please use Meson >= 0.61.0 or create a symlink @1@ -> @2@ in packaging'.format(
          get_option('prefix') / get_option('bindir') / exe,
          get_option('prefix') / multiarch_bindir / exe,
        )
      )
    endif
  endforeach
endif

if build_tests
    subdir('tests')
endif

# The following is an example for building internal marshallers that are used
# by GIO. We cannot guarantee glib-genmarshal availability while building GLib
# so they are pre-generated and placed into gmarshal-internal.[ch].
#
# gmarshal_internal = gnome.genmarshal('gmarshal-internal',
#   sources: 'gmarshal-internal.list',
#   prefix: '_g_cclosure_marshal',
#   valist_marshallers: true,
#   internal: true,
# )
