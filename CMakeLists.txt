cmake_minimum_required(VERSION 3.20)
project(MacroK VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(macro_k
    src/cli/main.cpp
    src/core/HookManager.cpp
    src/core/MacroPlayer.cpp
    src/core/MacroStorage.cpp
)

target_include_directories(macro_k PRIVATE src)

# Link Windows multimedia and user32 libraries
target_link_libraries(macro_k PRIVATE winmm user32)

set_target_properties(macro_k PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
