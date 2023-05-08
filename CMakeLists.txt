cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Echo)

add_library(Echo SHARED src/main.cpp "src/GUI/gui.cpp")

target_include_directories(Echo PRIVATE
  libraries/imgui-hook
  libraries/imgui-hook/imgui
  libraries/minhook/include
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
