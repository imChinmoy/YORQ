cmake_minimum_required(VERSION 3.20)

project(yorq
    VERSION 0.1.0
    DESCRIPTION "A custom embedded key-value storage engine"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


add_library(yorq_core
    src/core/record.cpp
    src/storage/wal.cpp
)

target_include_directories(yorq_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

add_executable(yorq
    src/main.cpp
    src/storage/file_storage.cpp
)

target_link_libraries(yorq PRIVATE yorq_core)

add_executable(record_test
    tests/record_test.cpp
)

target_link_libraries(record_test PRIVATE yorq_core)

enable_testing()
add_test(NAME record_test COMMAND record_test)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(yorq PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
    )
endif()
