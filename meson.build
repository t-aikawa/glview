
# glview

project('glview', 'c', 'cpp',
	version: '0.1.18',
  default_options: ['warning_level=0'])

# パッケージバージョン取得
version_glview = meson.project_version()
version_glview_arr = version_glview.split('.')
version_major = version_glview_arr[0].to_int()
version_minor = version_glview_arr[1].to_int()
version_patch = version_glview_arr[2].to_int()

# リンクするopenglを指定する
# gles  :opengles(libglesv2.so)
# opengl:opengl(libOpenGL.so)
opengl_option = get_option('opengl')
if opengl_option == 'gles'
opengl_type = 'opengles'
opengl_pkg = 'glesv2'
elif opengl_option == 'opengl'
opengl_type = 'opengl'
opengl_pkg = 'OpenGL'
else
opengl_type = 'opengl'
opengl_pkg = opengl_option
endif
message('glview:select opengl package:',opengl_pkg)

includes = include_directories('.','es1emu','glview','pthread')

config_h = configuration_data()
config_h.set('GLVIEW_PROJECT', 1)
config_h.set_quoted('PACKAGE_STRING', 'glview @0@'.format(version_glview))
config_h.set_quoted('PACKAGE_VERSION', version_glview)
config_h.set('GLV_VERSION_MAJOR', version_major)
config_h.set('GLV_VERSION_MINOR', version_minor)
config_h.set('GLV_VERSION_PATCH', version_patch)

if opengl_type == 'opengles'
config_h.set('_OPENGL__FIXED_FUNCTION_PIPLINE_SUPPORT', 0)
elif opengl_type == 'opengl'
config_h.set('_OPENGL__FIXED_FUNCTION_PIPLINE_SUPPORT', 1)
else
config_h.set('_OPENGL__FIXED_FUNCTION_PIPLINE_SUPPORT', 1)
endif

configure_file(output: 'config.h', configuration: config_h)

glview_opengl_h = configuration_data()
glview_opengl_h.set('opengl_lib', opengl_pkg)

if opengl_type == 'opengles'
glview_opengl_h.set('_OPENGL__FIXED_FUNCTION_PIPLINE_SUPPORT', 0)
glview_opengl_h.set('GLV_OPENGL_ES_SERIES', 1)
glview_opengl_h.set('GLV_OPENGL_GL_SERIES', 0)
elif opengl_type == 'opengl'
glview_opengl_h.set('_OPENGL__FIXED_FUNCTION_PIPLINE_SUPPORT', 1)
glview_opengl_h.set('GLV_OPENGL_ES_SERIES', 0)
glview_opengl_h.set('GLV_OPENGL_GL_SERIES', 1)
else
glview_opengl_h.set('_OPENGL__FIXED_FUNCTION_PIPLINE_SUPPORT', 1)
glview_opengl_h.set('GLV_OPENGL_ES_SERIES', 0)
glview_opengl_h.set('GLV_OPENGL_GL_SERIES', 1)
endif

configure_file( input : 'glview_opengl.h.in',
                output: 'glview_opengl.h',
                configuration: glview_opengl_h)

subdir('pthread')
subdir('es1emu')
subdir('glview')
subdir('clients')
