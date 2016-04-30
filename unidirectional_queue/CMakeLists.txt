cmake_minimum_required(VERSION 2.8)

project(unidirectional_queue)

file(GLOB HEADERS unidirectional_queue.h)
set(SOURCES unidirectional_queue.c)

add_library(unidirectional_queue STATIC ${HEADERS} ${SOURCES})



