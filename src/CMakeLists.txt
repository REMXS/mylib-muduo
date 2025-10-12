aux_source_directory(./ SRC)

# message("${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

add_library(mylib_copy_muduo SHARED ${SRC})

target_include_directories(mylib_copy_muduo PUBLIC ${PROJECT_SOURCE_DIR}/include)
