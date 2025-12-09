
find_package(GTest REQUIRED)

add_executable(Test)
target_sources(Test PRIVATE test.cc)


target_include_directories(Test PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(Test PRIVATE mylib_copy_muduo GTest::gtest GTest::gmock)


