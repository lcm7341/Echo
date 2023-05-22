cmake_minimum_required(VERSION 3.14)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Echo)

add_custom_target(
    increment_version_number
    COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/src/increment_version.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

file(
  GLOB_RECURSE SOURCE_FILES
  src/*.cpp
  src/*.hpp
)

add_library(Echo SHARED ${SOURCE_FILES})

add_dependencies(Echo
    increment_version_number
)

add_compile_definitions(Echo PRIVATE _CRT_SECURE_NO_WARNINGS)

include(cmake/CPM.cmake)

CPMAddPackage("gh:nlohmann/json@3.10.5")
CPMAddPackage("gh:HJfod/cocos-headers#bd2f1c0696255ed3455171f6042e04d99ad919e2")
CPMAddPackage("gh:TsudaKageyu/minhook#423d1e45af2ed2719a5c31e990e935ef301ed9c3")
CPMAddPackage("gh:matcool/gd.h#3b7e906c3ee32f3d718af67c11b9ddff9cf14896")

target_include_directories(Echo PRIVATE
  libraries/imgui-hook
  libraries/imgui-hook/imgui
  "${gd.h_SOURCE_DIR}"
  "${gd.h_SOURCE_DIR}/include"
)

add_subdirectory(libraries/imgui-hook)
target_link_libraries(Echo 
  cocos2d
  imgui-hook
  minhook
  opengl32
  nlohmann_json::nlohmann_json
  ${CMAKE_SOURCE_DIR}/libraries/glew32.lib
)

set_target_properties(Echo PROPERTIES DEBUG_POSTFIX "")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG:NONE /OPT:REF /OPT:ICF")
