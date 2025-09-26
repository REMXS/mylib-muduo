cmake_minimum_required(VERSION 3.15.0)
project(NetworkLibrary VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

aux_source_directory(${PROJECT_SOURCE_DIR}/src SRC)

find_package(PkgConfig REQUIRED)
# pkg_check_modules(ZLIB REQUIRED zlib)

add_executable(NetworkLibrary test.cc)

target_include_directories(NetworkLibrary PRIVATE ${SRC})