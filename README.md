###Requirements to run these experiments
####A compiler that supports C++11 features:
1. init'ing arrays
2. vector::data()
3. nullptr

####A nvidia card for nvidia specific extensions
1. GL_NV_shader_buffer_load
2. GL_NV_vertex_buffer_unified_memory

####A video card that supports these extensions
1.  GL_ARB_separate_shader_objects
2.  GL_ARB_timer_query
3.  GL_ARB_debug_output
4.  GL_ARB_draw_indirect
5.  GL_ARB_base_instance
6.  GL_ARB_shader_atomic_counters
7.  GL_ARB_clear_buffer_object
8.  GL_ARB_buffer_storage
9.  GL_ARB_map_buffer_range
10. GL_ARB_sync

####Dependencies
1. GLEW
2. GLFW
3. GLM

###How to build [GLEW](https://github.com/nigels-com/glew):
####Unix:
* make extensions (to generate src/glew.c [GLEW](https://github.com/nigels-com/glew))
* make
* sudo -s
* make install
* make clean

####Windows:
* use the project file in build/vc6/

###How to build [GLFW](https://github.com/glfw/glfw):
####Unix:
* cmake .
* make

####Windows:
* Link a new project to the make file


###Experiments
####bindless_nv
Compares bindless vbo's vrs traditional vbos
####indirect
Learning experiment on how to work with indirect rendering
####depth_buffer
Comparing traditional vs logrithmic writes to the depth buffer
####round_trip
Finding the amount of time it takes to send a texture to the gpu, write to it and then read it back
####raster_pattern
Renders GPU's rasterization pattern
####bits
Messing around with bit twiddling for the paper "Single Pass GPU Voxelization" section 6.
####scene_depth
Gets the used depth of the scene in a single draw call.
####single_pass_voxel
Implementing the paper "Single-Pass GPU Solid Voxelization for Real-Time Applications".
####glsl_derivative
A test to see if dFdx(position) and dFdy(position) are orthogonal
####wire_aa
Tests wether a proj matrix needs to be transposed for dot product ops, and finds pixel sizes at different depths.
####ogl_compute
Demo project to experiment with opengl compute shaders. Renders out a 2D fluid simulation.
####buffer_streaming
App to compare different buffer streaming techniques.
