# glview

cc = meson.get_compiler('c')
glib_2_0 = dependency('glib-2.0')
gthread_2_0 = dependency('gthread-2.0')
gobject_2_0 = dependency('gobject-2.0')
freetype2 = dependency('freetype2')
egl = dependency('egl')
wayland_client = dependency('wayland-client')
wayland_cursor = dependency('wayland-cursor')
wayland_egl = dependency('wayland-egl')
glesv2 = dependency('glesv2')
xkbcommon = dependency('xkbcommon')
im_ibus = dependency('ibus-1.0')
im_fcitx = dependency('fcitx-gclient')
dep_libm = cc.find_library('m')

scanner = dependency('wayland-scanner')
scanner_path = scanner.get_pkgconfig_variable('wayland_scanner')

# xdg-shell.xml
wayland_xdg_shell_client_protocol_h = custom_target('xdg-shell-client-protocol-header',
                                      output: 'xdg-shell-client-protocol.h',
                                      input: '/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml',
                                      command: [scanner_path, 'client-header' ,'@INPUT@', '@OUTPUT@'])

wayland_xdg_shell_protocol_c = custom_target('xdg-shell-protocol-code',
                                    output: 'xdg-shell-protocol.c',
                                    input:  '/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml',
                                    command: [scanner_path, 'private-code' ,'@INPUT@', '@OUTPUT@'])

# xdg-shell-unstable-v6.xml
wayland_xdg_shell_unstable_v6_client_protocol_h = custom_target('xdg-shell-unstable-v6-client-protocol-header',
                                      output: 'xdg-shell-unstable-v6-client-protocol.h',
                                      input: '/usr/share/wayland-protocols/unstable/xdg-shell/xdg-shell-unstable-v6.xml',
                                      command: [scanner_path, 'client-header' ,'@INPUT@', '@OUTPUT@'])

wayland_xdg_shell_unstable_v6_protocol_c = custom_target('xdg-shell-unstable-v6-protocol-code',
                                    output: 'xdg-shell-unstable-v6-protocol.c',
                                    input:  '/usr/share/wayland-protocols/unstable/xdg-shell/xdg-shell-unstable-v6.xml',
                                    command: [scanner_path, 'private-code' ,'@INPUT@', '@OUTPUT@'])
#

glview_lib = library(
	'glview', [
	'glview.c',
	'glview_widget.c',
	'glview_wayland.c',
	'glview_font.c',
	'glview_frame.c',
	'glview_gl.c',
	'glview_im_ibus.c',
	'glview_im_fcitx.c',
	'glview_part001.c',
	'glview_part002.c',
	'glview_part003.c',
	'glview_part004.c',
	'glview_part005.c',
	'glview_part006.c',
#	'xdg-shell-protocol.c',
#	'xdg-shell-unstable-v6-protocol.c',
	'ivi-application-protocol.c',
	'weston-client-window.c',
	wayland_xdg_shell_client_protocol_h, wayland_xdg_shell_protocol_c,
	wayland_xdg_shell_unstable_v6_client_protocol_h, wayland_xdg_shell_unstable_v6_protocol_c
	],
	include_directories: includes,
	link_with: [es1emu_lib , pthread_tool_lib],  # リンクするライブラリの指定
	dependencies : [glib_2_0,gthread_2_0,gobject_2_0, freetype2,egl,wayland_client,wayland_cursor,wayland_egl, glesv2,xkbcommon,im_ibus,im_fcitx,dep_libm])
	
	
	
glview_lib_dep = declare_dependency(include_directories : includes,
  link_with : glview_lib)	
	
