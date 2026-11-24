cmake_minimum_required(VERSION 3.24)
project(spout2_igpu_capture LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
add_compile_options(/utf-8 /EHsc)
add_compile_definitions(NOMINMAX UNICODE _UNICODE)

set(spout_sdk "${CMAKE_CURRENT_SOURCE_DIR}/third_party/spout/SPOUTSDK")
add_library(spout STATIC "${spout_sdk}/SpoutDirectX/SpoutDX/SpoutDX.cpp")
foreach(file SpoutCopy SpoutDirectX SpoutFrameCount SpoutSenderNames SpoutSharedMemory SpoutUtils)
  target_sources(spout PRIVATE "${spout_sdk}/SpoutGL/${file}.cpp")
endforeach()
target_include_directories(spout PUBLIC "${spout_sdk}/SpoutDirectX/SpoutDX")
target_link_libraries(spout PUBLIC d3d11 dxgi dxguid winmm version shlwapi shell32 advapi32 user32 gdi32 comdlg32)

set(obs_root "${CMAKE_CURRENT_SOURCE_DIR}/deps/obs")
set(OBS_RELEASE_CANDIDATE 0)
set(OBS_BETA 0)
configure_file("${obs_root}/libobs/obsconfig.h.in" "${CMAKE_BINARY_DIR}/obsconfig.h")
add_library(obs_sdk INTERFACE)
target_include_directories(obs_sdk INTERFACE "${obs_root}/libobs" "${CMAKE_BINARY_DIR}")
target_link_libraries(obs_sdk INTERFACE "${CMAKE_BINARY_DIR}/obs.lib")

add_library(warudo-spout-bridge MODULE bridge.cpp)
target_link_libraries(warudo-spout-bridge PRIVATE obs_sdk spout)

add_executable(check-bridge check.cpp)
target_link_libraries(check-bridge PRIVATE obs_sdk spout)
