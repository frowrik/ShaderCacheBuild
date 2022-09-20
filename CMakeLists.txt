cmake_minimum_required(VERSION 3.16)

project(ShaderCache C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/$<0:>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/project/intermediates")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/project/intermediates")

# tinyxml2
option(tinyxml2_BUILD_TESTING "" OFF)
set(BUILD_TESTING OFF)
set(tinyxml2_BUILD_TESTING OFF)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/deps/tinyxml2" tinyxml2)

# stb
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/deps/stb")

# dxc
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/deps/DirectXShaderCompilerBuild/inc")

# shader cache compiler
file(GLOB SHADERCOMP_FILES   "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
add_executable(ShaderCacheBin ${SHADERCOMP_FILES})
source_group(sources  FILES  ${SHADERCOMP_FILES})

target_link_libraries(ShaderCacheBin PRIVATE tinyxml2)
target_link_libraries(ShaderCacheBin PRIVATE d3dcompiler.lib)

set_target_properties(ShaderCacheBin PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/$<0:>")

add_custom_command(TARGET ShaderCacheBin POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/deps/DirectXShaderCompilerBuild/bin/x64/dxcompiler.dll" "${CMAKE_CURRENT_SOURCE_DIR}/build/$<0:>")
add_custom_command(TARGET ShaderCacheBin POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/deps/DirectXShaderCompilerBuild/bin/x64/dxil.dll" "${CMAKE_CURRENT_SOURCE_DIR}/build/$<0:>")
