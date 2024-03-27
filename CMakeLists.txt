cmake_minimum_required(VERSION 3.5)

set(BOOST_ROOT "F:/Boost")

add_subdirectory(lib/glew/build/cmake)
add_subdirectory(lib/glfw)

project(affine_transform)

# Используемый стандарт C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIREDON)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED COMPONENTS program_options)
find_package(OpenGl REQUIRED)
 
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

# Если boost найден, то
if (Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS}) # Подключаем заголовочные файлы

  add_executable(${PROJECT_NAME}
    src/main.cpp
  ) # Создаем исполняемый файл проекта

  target_include_directories(${PROJECT_NAME} PRIVATE
    lib/glew/include
  )

  target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} ${OPENGL_LIBRARIES} glfw glew) # Подключаем библиотеки
endif()