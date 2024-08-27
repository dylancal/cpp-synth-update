cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(cpp-synth CXX)

find_package(PkgConfig)
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(portaudio CONFIG REQUIRED)
find_package(OpenGL REQUIRED)




add_executable(cpp-synth
  cpp-synth/main.cpp
  cpp-synth/Synth.cpp
  cpp-synth/wavetable.cpp
  imgui/backends/imgui_impl_glfw.cpp
  imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(cpp-synth PRIVATE
	cpp-synth/
)

target_link_libraries(cpp-synth PRIVATE
  glfw
  imgui::imgui
  portaudio
  OpenGL::GL
)
