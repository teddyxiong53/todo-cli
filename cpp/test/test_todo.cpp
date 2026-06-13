#include "../src/todo.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

// 简单测试框架
static int tests_passed = 0;
static int tests_failed = 0;

#define RUN_TEST(name, func)                                              \
    do {                                                                  \
        std::cout << "  TEST: " << name << " ... ";                       \
        if (func()) {                                                     \
            std::cout << "PASS" << std::endl;                             \
            tests_passed++;                                               \
        } else {                                                          \
            std::cout << "FAIL" << std::endl;                             \
            tests_failed++;                                               \
        }                                                                 \
    } while (0)

static std::string temp_file() {
    return (fs::temp_directory_path() / "todo_test_XXXXXX.json").string();
}

// ── 测试函数 ──────────────────────────────────────────────────────

static bool t_add_todo() {
    std::string path = temp_file();
    {
        TodoStorage s(path);
        s.add("测试任务1");
    }
    // 重新加载验证没有崩溃
    TodoStorage s(path);
    int ret = s.list();
    fs::remove(path);
    return ret == 0;
}

static bool t_list_empty() {
    std::string path = temp_file();
    TodoStorage s(path);
    std::stringstream buf;
    std::streambuf* old = std::cout.rdbuf(buf.rdbuf());
    int ret = s.list();
    std::cout.rdbuf(old);
    fs::remove(path);
    return ret == 0 && buf.str().find("暂无任务") != std::string::npos;
}

static bool t_done() {
    std::string path = temp_file();
    {
        TodoStorage s(path);
        s.add("可完成的任务");
    }
    {
        TodoStorage s(path);
        int ret = s.done(1);
        fs::remove(path);
        return ret == 0;
    }
}

static bool t_done_already_completed() {
    std::string path = temp_file();
    {
        TodoStorage s(path);
        s.add("完成的任务");
        s.done(1);
    }
    {
        TodoStorage s(path);
        std::stringstream buf;
        std::streambuf* old = std::cout.rdbuf(buf.rdbuf());
        int ret = s.done(1);
        std::cout.rdbuf(old);
        fs::remove(path);
        return ret == 0 && buf.str().find("已经是完成状态") != std::string::npos;
    }
}

static bool t_delete() {
    std::string path = temp_file();
    {
        TodoStorage s(path);
        s.add("待删除");
    }
    {
        TodoStorage s(path);
        int ret = s.remove(1);
        fs::remove(path);
        return ret == 0;
    }
}

static bool t_edit() {
    std::string path = temp_file();
    {
        TodoStorage s(path);
        s.add("旧标题");
    }
    {
        TodoStorage s(path);
        int ret = s.edit(1, "新标题");
        fs::remove(path);
        return ret == 0;
    }
}

static bool t_id_not_found() {
    std::string path = temp_file();
    TodoStorage s(path);
    int ret = s.done(999);
    fs::remove(path);
    return ret == 1;
}

static bool t_clear_done() {
    std::string path = temp_file();
    {
        TodoStorage s(path);
        s.add("待完成");
        s.add("已完成");
        s.done(2);
    }
    {
        TodoStorage s(path);
        int ret = s.clear_done();
        fs::remove(path);
        return ret == 0;
    }
}

static bool t_clear_idempotent() {
    std::string path = temp_file();
    {
        TodoStorage s(path);
        s.add("只有未完成");
    }
    {
        TodoStorage s(path);
        std::stringstream buf;
        std::streambuf* old = std::cout.rdbuf(buf.rdbuf());
        int ret = s.clear_done();
        std::cout.rdbuf(old);
        fs::remove(path);
        return ret == 0 && buf.str().find("没有已完成的任务需要清除") != std::string::npos;
    }
}

static bool t_file_not_exists() {
    std::string path = "/tmp/todo_test_nonexistent_should_be_ok.json";
    TodoStorage s(path);
    int ret = s.list();
    fs::remove(path);
    return ret == 0;
}

// ── 主函数 ────────────────────────────────────────────────────────

int main() {
    std::cout << "=== C++ Todo CLI 测试 ===" << std::endl;

    RUN_TEST("add todo",          t_add_todo);
    RUN_TEST("list empty",        t_list_empty);
    RUN_TEST("done",              t_done);
    RUN_TEST("done already completed", t_done_already_completed);
    RUN_TEST("delete",            t_delete);
    RUN_TEST("edit",              t_edit);
    RUN_TEST("id not found",      t_id_not_found);
    RUN_TEST("clear done",        t_clear_done);
    RUN_TEST("clear idempotent",  t_clear_idempotent);
    RUN_TEST("file not exists",   t_file_not_exists);

    std::cout << std::endl;
    std::cout << "结果：" << tests_passed << " 通过"
              << (tests_failed > 0 ? ", " + std::to_string(tests_failed) + " 失败" : "")
              << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
