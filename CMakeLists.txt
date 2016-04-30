cmake_minimum_required(VERSION 2.8)

set(PROJ_PULLOWRD pullwords)

project(${PROJ_PULLOWRD} C)
set(SOURCE_FILES main.c main.h)
set(CMAKE_C_FLAGS "-std=c99")

include_directories(/usr/include /usr/include/x86_64-linux-gnu)
add_subdirectory(cJSON)
include_directories(cJSON)
link_libraries(cJSON)
add_subdirectory(unidirectional_queue)
include_directories(unidirectional_queue)
link_libraries(unidirectional_queue)
add_executable(pullwords ${SOURCE_FILES})

target_link_libraries(pullwords hiredis )
target_link_libraries(pullwords pcre2-8)
target_link_libraries(pullwords pthread)
