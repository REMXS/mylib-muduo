cmake_minimum_required(VERSION 3.15.0)
project(NetworkLibrary VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)



find_package(PkgConfig REQUIRED)
# pkg_check_modules(ZLIB REQUIRED zlib)



set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY  ${PROJECT_SOURCE_DIR}/lib)




add_subdirectory(src)
add_subdirectory(test)