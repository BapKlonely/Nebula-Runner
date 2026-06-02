cmake_minimum_required(VERSION 3.20)

project(NebulaRunner
  VERSION 1.0.0
  DESCRIPTION "A polished terminal mini game built with C++ and FTXUI"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
  GIT_TAG v5.0.0
)

FetchContent_MakeAvailable(ftxui)

add_executable(nebula_runner
  src/main.cpp
  src/game.cpp
)

target_include_directories(nebula_runner PRIVATE src)

target_link_libraries(nebula_runner
  PRIVATE
    ftxui::screen
    ftxui::dom
    ftxui::component
)

if(MSVC)
  target_compile_options(nebula_runner PRIVATE /W4)
else()
  target_compile_options(nebula_runner PRIVATE -Wall -Wextra -pedantic)
endif()
