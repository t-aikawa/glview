# glview_sample

cc = meson.get_compiler('c')
dep_libm = cc.find_library('m')
glesv2 = dependency('glesv2')

executable(
    'glview_test', [
	'test.c',
	'test1.c',
	'test2.c',
	'test3.c'
    ],
    include_directories: includes,  # インクルードディレクトリの指定
    link_with: [glview_lib , es1emu_lib , pthread_tool_lib],  # リンクするライブラリの指定
	dependencies : [glesv2,dep_libm])

executable(
    'smoke', [
	'smoke.c'
    ],
    include_directories: includes,  # インクルードディレクトリの指定
    link_with: [glview_lib , es1emu_lib , pthread_tool_lib],  # リンクするライブラリの指定
	dependencies : [glesv2,dep_libm])

executable(
    'simple-egl', [
	'simple-egl.c'
    ],
    include_directories: includes,  # インクルードディレクトリの指定
    link_with: [glview_lib , es1emu_lib , pthread_tool_lib],  # リンクするライブラリの指定
	dependencies : [glesv2,dep_libm])

executable(
    's001', [
	's001.c'
    ],
    include_directories: includes,  # インクルードディレクトリの指定
    link_with: [glview_lib , es1emu_lib , pthread_tool_lib],  # リンクするライブラリの指定
	dependencies : [glesv2,dep_libm])
