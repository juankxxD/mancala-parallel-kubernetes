cmake_minimum_required(VERSION 3.16)
project(mancala_motor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(OpenMP REQUIRED)
find_package(Threads REQUIRED)

# --- Biblioteca con la lógica del motor (reutilizada por todos los binarios) ---
add_library(mancala_lib STATIC
    board.cpp
    alphabeta.cpp
    api.cpp
    http_server.cpp
)
target_include_directories(mancala_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mancala_lib
    PUBLIC OpenMP::OpenMP_CXX Threads::Threads
)
if(WIN32)
    target_link_libraries(mancala_lib PUBLIC ws2_32)
endif()

if(MSVC)
    target_compile_options(mancala_lib PRIVATE /O2 /W4)
else()
    target_compile_options(mancala_lib PRIVATE -O2 -Wall -Wextra)
endif()

# --- Binario principal: servidor HTTP ---
add_executable(mancala_motor main.cpp)
target_link_libraries(mancala_motor PRIVATE mancala_lib)

# --- Binario de benchmark CLI ---
add_executable(mancala_bench bench_main.cpp)
target_link_libraries(mancala_bench PRIVATE mancala_lib)

# --- Tests unitarios ---
enable_testing()
add_executable(mancala_tests ${CMAKE_CURRENT_SOURCE_DIR}/../tests/test_runner.cpp)
target_link_libraries(mancala_tests PRIVATE mancala_lib)
add_test(NAME mancala_unit_tests COMMAND mancala_tests)
