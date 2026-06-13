add_executable(test_todo test_todo.cpp "${CMAKE_SOURCE_DIR}/src/todo.cpp")
target_include_directories(test_todo PRIVATE "${CMAKE_SOURCE_DIR}/include")
add_test(NAME test_todo COMMAND test_todo)
