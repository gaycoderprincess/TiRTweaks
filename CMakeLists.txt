cmake_minimum_required(VERSION 3.27)
project(TIRTweaks)

SET(CMAKE_SYSTEM_NAME Windows)

SET(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
SET(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
SET(CMAKE_RC_COMPILER i686-w64-mingw32-windres)
SET(CMAKE_RANLIB i686-w64-mingw32-ranlib)

SET(CMAKE_CXX_STANDARD 20)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -masm=intel -s -static")

include_directories(../nya-common/3rdparty/imgui)
include_directories(../nya-common/3rdparty/imgui_dx8)

add_compile_definitions(IMGUI_USER_CONFIG="../../nya_imconfig.h" NYA_BACKEND_DX8 NYA_NO_FONT)
add_library(TIRTweaks SHARED
        ../nya-common/3rdparty/imgui/imgui.cpp
        ../nya-common/3rdparty/imgui_dx8/imgui_impl_dx8.cpp
        ../nya-common/3rdparty/imgui/imgui_draw.cpp
        ../nya-common/3rdparty/imgui/imgui_tables.cpp
        ../nya-common/3rdparty/imgui/imgui_widgets.cpp
        ../nya-common/nya_commonfuncs.cpp
        ../nya-common/nya_commondraw.cpp
        ../nya-common/nya_dx8_hookbase.cpp
        main.cpp)
target_include_directories(TIRTweaks PUBLIC ../nya-common)
target_include_directories(TIRTweaks PUBLIC ../nya-common/3rdparty)
target_link_options(TIRTweaks PRIVATE "-Wl,--exclude-all-symbols")

set_target_properties(TIRTweaks PROPERTIES PREFIX "")
set_target_properties(TIRTweaks PROPERTIES SUFFIX "_gcp.dll")