cmake_minimum_required (VERSION 3.0)
project(toucan)

# dependencies ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

set(OpenGL_GL_PREFERENCE "GLVND")

find_package(glfw3 3.2 REQUIRED)
find_package(Vulkan REQUIRED)

set(CMAKE_CXX_FLAGS "-fdiagnostics-color=always -Wall -Wextra")
set(CMAKE_C_FLAGS "-g -fdiagnostics-color=always -Wall -Wextra")

# vulkan settings ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include_directories(${Vulkan_INCLUDE_DIR})
set(ENV{VK_INSTANCE_LAYERS} VK_LAYER_LUNARG_standard_validation)

# shader compilation ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)
add_custom_command(
  OUTPUT 
    ${CMAKE_BINARY_DIR}/vertex.vert.sipr 
    ${CMAKE_BINARY_DIR}/vertex.vert.sipr.text
    ${CMAKE_BINARY_DIR}/fragment.frag.sipr 
    ${CMAKE_BINARY_DIR}/fragment.frag.sipr.text 
  COMMAND glslangValidator -H -o ${CMAKE_BINARY_DIR}/vertex.vert.sipr ${SHADER_DIR}/vertex.vert > ${CMAKE_BINARY_DIR}/vertex.vert.sipr.text
  COMMAND glslangValidator -H -o ${CMAKE_BINARY_DIR}/fragment.frag.sipr ${SHADER_DIR}/fragment.frag > ${CMAKE_BINARY_DIR}/fragment.frag.sipr.text
  DEPENDS 
    ${SHADER_DIR}/vertex.vert 
    ${SHADER_DIR}/fragment.frag
)

add_custom_target(shaders DEPENDS ${CMAKE_BINARY_DIR}/fragment.frag.sipr ${CMAKE_BINARY_DIR}/vertex.vert.sipr)

# build ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_executable(toucan main.c toucan.c spock.c nets.c graphics.c layout.c link.c)
add_dependencies(toucan shaders)
target_link_libraries(toucan glfw m dl)
