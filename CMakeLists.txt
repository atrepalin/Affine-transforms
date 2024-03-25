cmake_minimum_required(VERSION 3.5)

project(affine_transform)

# Используемый стандарт C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIREDON)
 
if (CMAKE_CXX_COMPILER_IDSTREQUAL"GNU")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
add_compile_options(
    -Werror=return-type
    -Wfatal-errors
    -Wall
    -Wextra
    -Wwrite-strings
    -Winit-self
  )
elseif (CMAKE_CXX_COMPILER_IDSTREQUAL"MSVC")
add_compile_options("/W4")
endif()


add_executable(${PROJECT_NAME}
  src/main.cpp
)