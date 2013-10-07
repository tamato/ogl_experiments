###Requires a compiler that supports C++11 features:
1. init'ing arrays
2. vector::data()

###An nvidia card for nvidia specific extensions
1. GL_NV_shader_buffer_load
2. GL_NV_vertex_buffer_unified_memory

###A video card that supports these extensions
1. GL_ARB_separate_shader_objects
2. GL_ARB_timer_query
3. GL_ARB_debug_output

###How to build GLEW (from https://github.com/nigels-com/glew):
####Unix:
* make
* sudo -s
* make install
* make clean

####Windows:
* use the project file in build/vc6/

###How to build GLFW:
please refer to [glfw on github](https://github.com/glfw/glfw) for details, but a quick summary is as follows
####Unix:
* cmake .
* make

####Windows:
* Link a new project to the make file
