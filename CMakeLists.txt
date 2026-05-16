cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXTRA_COMPONENT_DIR components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(BandHero-bass)
