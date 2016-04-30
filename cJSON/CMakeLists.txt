cmake_minimum_required(VERSION 2.8)

project(cJSON)

file(GLOB HEADERS cJSON.h)
set(SOURCES cJSON.c)

add_library(cJSON STATIC ${HEADERS} ${SOURCES})

target_link_libraries(cJSON m)


