# es1emu

cc = meson.get_compiler('c')
opengl = dependency(opengl_pkg)
thread_dep = dependency('threads')
dep_libm = cc.find_library('m')

es1emu_lib = library(
	'es1emu', [
	'es1emu_emulation.c',
	'es1emu_matrix.c',
	'es1emu_shader.c'
	],
	include_directories: includes,
	dependencies : [opengl,thread_dep,dep_libm])
	
glview_es1emu_lib_dep = declare_dependency(include_directories : includes,
  link_with : es1emu_lib)
  
