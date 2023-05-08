cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Echo)

add_library(Echo SHARED src/main.cpp "src/GUI/gui.cpp" "src/Hooks/hooks.cpp" "src/Logic/logic.cpp" "src/Logic/macros.cpp")

target_include_directories(Echo PRIVATE
  libraries/imgui-hook
  libraries/imgui-hook/imgui
  libraries/minhook/include
  libraries/gd.h/include
  libraries/gd.h
)

add_subdirectory(libraries/minhook)
add_subdirectory(libraries/imgui-hook)
add_subdirectory(libraries/cocos-headers)
target_link_libraries(Echo 
  cocos2d
  imgui-hook
  minhook
  opengl32
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s")


set_target_properties(Echo PROPERTIES DEBUG_POSTFIX "")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG:NONE /OPT:REF /OPT:ICF")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG:NONE /OPT:REF /OPT:ICF")
endif()