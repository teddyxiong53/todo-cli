cmake_minimum_required(VERSION 3.14)
project(todo CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── 依赖：nlohmann/json（本地头文件） ──
set(NLOHMANN_JSON_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# ── 主程序 ──
add_executable(todo src/main.cpp src/todo.cpp)
target_include_directories(todo PRIVATE ${NLOHMANN_JSON_INCLUDE})

# ── 测试 ──
enable_testing()
add_subdirectory(test)
