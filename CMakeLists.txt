cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
# http://www.cmake.org/Wiki/CMake/Policies
cmake_policy(VERSION 2.8)

project( performance_experiments )

set( performance_experiments_VERSION_MAJOR 1 )
set( performance_experiments_VERSION_MINOR 0 )

#if(WIN32)
#  set( CMAKE_GENERATOR "Visual Studio 11" )
#  set(CMAKE_CXX_COMPILER "")
#  set(CMAKE_CC_COMPILER "")
#endif(WIN32)

############################################
# set up to use clang
#set(CMAKE_CC_COMPILER "/usr/local/bin/clang")
#set(CMAKE_CXX_COMPILER "/usr/local/bin/clang++")

############################################
# Flags for C++11 with g++
#list( APPEND CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS} -g -ftest-coverage -fprofile-arcs")
list( APPEND CMAKE_CXX_FLAGS "-std=c++11 -static-libstdc++")

# staticly link in libstdc++
set(CMAKE_EXE_LINKER_FLAGS "-static-libstdc++ ${CMAKE_EXE_LINKER_FLAGS}")

#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")

message("compiler: " ${CMAKE_CXX_COMPILER})

############################################
# move exe's into bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

################################
# Add common source
set(COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/common")
include_directories(${COMMON_DIR})

set(DATA "${CMAKE_CURRENT_SOURCE_DIR}/data")
include_directories(${DATA})

################################
# Add OpenGL
find_package(OpenGL REQUIRED)
if(NOT OPENGL_FOUND)
    message("ERROR: OpenGL not found")
endif(NOT OPENGL_FOUND)

if(WIN32)
    set(GL_LIBRARY OpenGL32)
else(WIN32)
    set(GL_LIBRARY GL)
endif(WIN32)

###########################################
# Add GLEW
set(GLEW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/glew)
include_directories(${GLEW_DIR}/include)
link_directories(${GLEW_DIR}/lib)
add_definitions(-DGLEW_STATIC)
if(WIN32)
    set(GLEW_LIBRARY glew32s)
else(WIN32)
    set(GLEW_LIBRARY GLEW)
endif(WIN32)

###########################################
# Add GLFW3, http://www.glfw.org/docs/latest/build.html
# Using method: With CMake on Unix and -installed- GLFW binaries
# to install glfw use: 
#   make ; sudo make install
# the install directories are: 
#   /usr/local/include/GLFW
#   /usr/local/lib/cmake/glfw
#   /usr/local/lib/pkgconfig/glfw3.pc
#   /usr/local/lib/libglfw3.a
# first make sure we can use the pkg_search_module command
find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
include_directories(${GLFW_INCLUDE_DIRS})

# set(GLFW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw)
# add_subdirectory(${GLFW_DIR})
# include_directories(${GLFW_DIR}/include)
# link_directories(${GLEW_DIR}/src) # glfw does not create lib/, but stores it's lib in src/
# if(WIN32)
#     set(GLFW_LIBRARY glfw3dll)
# else(WIN32)
#     set(GLFW_LIBRARY libglfw3.so)
# endif(WIN32)

###########################################
# Add GLM
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/glm)

################################
# Add libraries to executables
if(APPLE)
    set(LIBRARY_FILES ${OPENGL_LIBRARIES})
elseif(WIN32) # not right, need to fix on a windows machine
    set(LIBRARY_FILES ${GLEW_LIBRARY} ${GL_LIBRARY} ${GLFW_STATIC_LIBRARIES})
else(APPLE)
    set(LIBRARY_FILES ${GLFW_STATIC_LIBRARIES} ${GLEW_LIBRARY} ${GL_LIBRARY} )
endif(APPLE)

################################
# function to create a project for the sample in the solution
function(createExperiment NAME)
    # set(DATA_DIR ${CMAKE_BINARY_DIR}/bin/${NAME}_data)
    # file(MAKE_DIRECTORY ${DATA_DIR})

    file(GLOB COMMON_SOURCE ${COMMON_DIR}/*.cpp)
    file(GLOB PROJECT_SOURCE *.cpp)
    file(GLOB PROJECT_INLINE *.inl)
    file(GLOB PROJECT_HEADER *.hpp *.h)
    file(GLOB PROJECT_SHADER *.vert *.frag *.geom *.cont *.eval *.glsl *.compute)

    source_group("Common Files" FILES ${COMMON_DIR})
    source_group("Inline Files" FILES ${PROJECT_INLINE})
    source_group("Shader Files" FILES ${PROJECT_SHADER})

    add_executable(${NAME} WIN32 ${COMMON_SOURCE} ${PROJECT_SOURCE} ${PROJECT_INLINE} ${PROJECT_HEADER} ${PROJECT_TEXT})

    target_link_libraries(${NAME} ${LIBRARY_FILES})
endfunction(createExperiment)

############################################
# Add in the source that will make a demo
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/experiments)
