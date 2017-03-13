copy ..\__build__\x86\Debug\lib\intl.lib ..\..\frida\build\sdk-windows\Win32-Debug\lib\
copy ..\__build__\x86\Debug\lib\intl.pdb ..\..\frida\build\sdk-windows\Win32-Debug\lib\
copy ..\__build__\x86\Debug\lib\glib-2.0.lib ..\..\frida\build\sdk-windows\Win32-Debug\lib\
copy ..\__build__\x86\Debug\lib\glib-2.0.pdb ..\..\frida\build\sdk-windows\Win32-Debug\lib\
copy ..\__build__\x86\Debug\lib\gobject-2.0.lib ..\..\frida\build\sdk-windows\Win32-Debug\lib\
copy ..\__build__\x86\Debug\lib\gobject-2.0.pdb ..\..\frida\build\sdk-windows\Win32-Debug\lib\
copy ..\__build__\x86\Debug\lib\gmodule-2.0.lib ..\..\frida\build\sdk-windows\Win32-Debug\lib\
copy ..\__build__\x86\Debug\lib\gmodule-2.0.pdb ..\..\frida\build\sdk-windows\Win32-Debug\lib\
copy ..\__build__\x86\Debug\lib\gthread-2.0.lib ..\..\frida\build\sdk-windows\Win32-Debug\lib\
copy ..\__build__\x86\Debug\lib\gthread-2.0.pdb ..\..\frida\build\sdk-windows\Win32-Debug\lib\
copy ..\__build__\x86\Debug\lib\gio-2.0.lib ..\..\frida\build\sdk-windows\Win32-Debug\lib\
copy ..\__build__\x86\Debug\lib\gio-2.0.pdb ..\..\frida\build\sdk-windows\Win32-Debug\lib\

copy ..\__build__\x86_64\Debug\lib\intl.lib ..\..\frida\build\sdk-windows\x64-Debug\lib\
copy ..\__build__\x86_64\Debug\lib\intl.pdb ..\..\frida\build\sdk-windows\x64-Debug\lib\
copy ..\__build__\x86_64\Debug\lib\glib-2.0.lib ..\..\frida\build\sdk-windows\x64-Debug\lib\
copy ..\__build__\x86_64\Debug\lib\glib-2.0.pdb ..\..\frida\build\sdk-windows\x64-Debug\lib\
copy ..\__build__\x86_64\Debug\lib\gobject-2.0.lib ..\..\frida\build\sdk-windows\x64-Debug\lib\
copy ..\__build__\x86_64\Debug\lib\gobject-2.0.pdb ..\..\frida\build\sdk-windows\x64-Debug\lib\
copy ..\__build__\x86_64\Debug\lib\gmodule-2.0.lib ..\..\frida\build\sdk-windows\x64-Debug\lib\
copy ..\__build__\x86_64\Debug\lib\gmodule-2.0.pdb ..\..\frida\build\sdk-windows\x64-Debug\lib\
copy ..\__build__\x86_64\Debug\lib\gthread-2.0.lib ..\..\frida\build\sdk-windows\x64-Debug\lib\
copy ..\__build__\x86_64\Debug\lib\gthread-2.0.pdb ..\..\frida\build\sdk-windows\x64-Debug\lib\
copy ..\__build__\x86_64\Debug\lib\gio-2.0.lib ..\..\frida\build\sdk-windows\x64-Debug\lib\
copy ..\__build__\x86_64\Debug\lib\gio-2.0.pdb ..\..\frida\build\sdk-windows\x64-Debug\lib\

rmdir /S /Q ..\..\frida\build\sdk-windows\Win32-Debug\include\glib-2.0\
rmdir /S /Q ..\..\frida\build\sdk-windows\Win32-Debug\include\gio-win32-2.0\
xcopy /E ..\__build__\x86\Debug\include\glib-2.0 ..\..\frida\build\sdk-windows\Win32-Debug\include\glib-2.0\
xcopy /E ..\__build__\x86\Debug\include\gio-win32-2.0 ..\..\frida\build\sdk-windows\Win32-Debug\include\gio-win32-2.0\

rmdir /S /Q ..\..\frida\build\sdk-windows\x64-Debug\include\glib-2.0\
rmdir /S /Q ..\..\frida\build\sdk-windows\x64-Debug\include\gio-win32-2.0\
xcopy /E ..\__build__\x86_64\Debug\include\glib-2.0 ..\..\frida\build\sdk-windows\x64-Debug\include\glib-2.0\
xcopy /E ..\__build__\x86_64\Debug\include\gio-win32-2.0 ..\..\frida\build\sdk-windows\x64-Debug\include\gio-win32-2.0\
