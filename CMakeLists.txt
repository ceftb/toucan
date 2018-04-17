cmake_minimum_required (VERSION 3.0)
project(toucan)

# dependencies ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

set(OpenGL_GL_PREFERENCE "GLVND")

find_package(glfw3 3.2 REQUIRED)
find_package(Vulkan REQUIRED)
#find_package(OpenGL REQUIRED)

set(CMAKE_CXX_FLAGS "-fdiagnostics-color=always -Wall -Wextra")
set(CMAKE_C_FLAGS "-fdiagnostics-color=always -Wall -Wextra")

# vulkan settings ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_directories(${Vulkan_INCLUDE_DIR})
set(ENV{VK_INSTANCE_LAYERS} VK_LAYER_LUNARG_standard_validation)

# build ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#add_executable(toucan main.c glad.c)
#target_link_libraries(toucan ${OPENGL_gl_LIBRARY} glfw)

add_executable(vtoucan vmain.c)
target_link_libraries(vtoucan glfw)
