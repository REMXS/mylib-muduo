
add_executable(Test test.cc)

target_include_directories(Test PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(Test PUBLIC mylib_copy_muduo)

