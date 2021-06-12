# pthread_tool

cc = meson.get_compiler('c')
thread_dep = dependency('threads')

pthread_tool_lib = library(
	'pthread_tool', [
	'pthread_msq.c',
	'pthread_timer.c'    
	],
	include_directories: includes,
	dependencies : [thread_dep])
	
glview_pthread_tool_lib_dep = declare_dependency(include_directories : includes,
  link_with : pthread_tool_lib)
	
