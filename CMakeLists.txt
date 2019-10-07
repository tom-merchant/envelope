cmake_minimum_required(VERSION 3.15)
project(envelope C)

set(CMAKE_C_STANDARD 99)

add_library(envelope SHARED envelope.c envelope.h)