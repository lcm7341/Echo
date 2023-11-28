cmake_minimum_required(VERSION 3.14)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Echo)
set(Echo_VERSION_MAJOR 1)
set(Echo_VERSION_MINOR 1)

set(CMAKE_SHARED_LIBRARY_SUFFIX " v${Echo_VERSION_MAJOR}.${Echo_VERSION_MINOR}.dll")

add_custom_target(
    increment_version_number
    COMMAND py ${CMAKE_CURRENT_SOURCE_DIR}/src/increment_version.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

file(
  GLOB_RECURSE SOURCE_FILES
  src/*.cpp
  src/*.hpp
)

add_library(Echo SHARED ${SOURCE_FILES}  "src/Logic/speedhack.h" "src/Logic/speedhack.cpp" "src/Clickbot/clickbot.hpp" "src/Clickbot/clickbot.cpp" "src/Logic/Conversions/mhr.h" "src/GUI/implot/implot.h" "src/GUI/implot/implot_internal.h" "src/Logic/Conversions/json.h" "src/Logic/Conversions/zbotf.h"      )

add_dependencies(Echo
    increment_version_number
)

add_compile_definitions(Echo PRIVATE _CRT_SECURE_NO_WARNINGS)

include(cmake/CPM.cmake)

CPMAddPackage("gh:nlohmann/json@3.10.5")
CPMAddPackage("gh:HJfod/cocos-headers#bd2f1c0696255ed3455171f6042e04d99ad919e2")
CPMAddPackage("gh:TsudaKageyu/minhook#423d1e45af2ed2719a5c31e990e935ef301ed9c3")
CPMAddPackage("gh:maxnut/gd.h#244f3e193fcb708d6440bf2420b94fc384f48f08")
CPMAddPackage("gh:simdjson/simdjson@3.2.2")

target_include_directories(Echo PRIVATE
  libraries/imgui-hook
  libraries/imgui-hook/imgui
  libraries/imguifiledialog
  "${gd.h_SOURCE_DIR}"
  "${gd.h_SOURCE_DIR}/include"
)

add_subdirectory(libraries/imgui-hook)
add_subdirectory(libraries/imguifiledialog)
add_subdirectory(libraries/fmod-headers)

target_link_libraries(Echo 
  cocos2d
  imgui-hook
  minhook
  ImGuiFileDialog
  opengl32
  nlohmann_json::nlohmann_json
  simdjson
  ${CMAKE_SOURCE_DIR}/libraries/glew32.lib
  fmod
)

set_target_properties(Echo PROPERTIES DEBUG_POSTFIX "")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG:NONE /OPT:REF /OPT:ICF")