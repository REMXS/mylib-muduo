
add_executable(Test test.cc)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTEST REQUIRED gtest_main)


target_include_directories(Test PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(Test PUBLIC mylib_copy_muduo)



target_compile_options(Test PRIVATE ${GTEST_CFLAGS})
target_link_libraries(Test PRIVATE ${GTEST_LDFLAGS})