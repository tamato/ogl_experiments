cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
# http://www.cmake.org/Wiki/CMake/Policies
cmake_policy(VERSION 2.8)

project( performance_experiments )

set( performance_experiments_VERSION_MAJOR 1 )
set( performance_experiments_VERSION_MINOR 0 )

############################################
# Flags for C++11 with g++
list( APPEND CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS} -g -ftest-coverage -fprofile-arcs")

#set(CMAKE_BUILD_TYPE DEBUG)
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")

############################################
# move exe's into bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

################################
# Add common source
set(COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/common")
include_directories(${COMMON_DIR})

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
# Add GLFW3
# GLFW was built with BUILD_SHARED_LIBS to ON
set(GLFW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw)
include_directories(${GLFW_DIR}/include)
link_directories(${GLFW_DIR}/src)

set(GLFW_LIBRARY glfw)

###########################################
# Add GLM
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/glm)

################################
# Add libraries to executables
if(APPLE)
    set(BINARY_FILES ${OPENGL_LIBRARIES})
else(APPLE)
    set(BINARY_FILES ${GLFW_LIBRARY} ${GLEW_LIBRARY} ${GL_LIBRARY})
endif(APPLE)

################################
# function to create a project for the sample in the solution
function(createExperiment NAME)
    set(DATA_DIR ${CMAKE_BINARY_DIR}/bin/${NAME}_data)
    file(MAKE_DIRECTORY ${DATA_DIR})

    file(GLOB COMMON_SOURCE ${COMMON_DIR}/*.cpp)
    file(GLOB PROJECT_SOURCE *.cpp)
    file(GLOB PROJECT_INLINE *.inl)
    file(GLOB PROJECT_HEADER *.hpp *.h)
    file(GLOB PROJECT_SHADER *.vert *.frag *.geom *.cont *.eval *.glsl)

    source_group("Common Files" FILES ${COMMON_DIR})
    source_group("Inline Files" FILES ${PROJECT_INLINE})
    source_group("Shader Files" FILES ${PROJECT_SHADER})

    add_executable(${NAME} ${COMMON_SOURCE} ${PROJECT_SOURCE} ${PROJECT_INLINE} ${PROJECT_HEADER} ${PROJECT_TEXT})
    foreach(SHADER_FILE ${PROJECT_SHADER})
        add_custom_command(
            TARGET ${NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${SHADER_FILE} ${DATA_DIR}
        )
    endforeach()

    target_link_libraries(${NAME} ${BINARY_FILES})
endfunction(createExperiment)

############################################
# Add in the source that will make a demo
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/experiments)

