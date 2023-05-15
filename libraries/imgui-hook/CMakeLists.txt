cmake_minimum_required(VERSION 3.0)

add_library(imgui-hook STATIC
    imgui-hook.cpp
    imgui/imgui.cpp
	imgui/imgui_widgets.cpp
	imgui/imgui_tables.cpp
    imgui/imgui_draw.cpp
    imgui/backends/imgui_impl_win32.cpp
    imgui/backends/imgui_impl_opengl3.cpp
    imgui/misc/cpp/imgui_stdlib.cpp
)

set_property(TARGET imgui-hook PROPERTY CXX_STANDARD 17)

CPMAddPackage("gh:HJfod/cocos-headers#bd2f1c0696255ed3455171f6042e04d99ad919e2")
target_link_libraries(imgui-hook PRIVATE cocos2d)

target_include_directories(imgui-hook PRIVATE imgui)