#include "todo.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

static std::string default_filepath() {
    const char* home = std::getenv("HOME");
    if (!home) home = std::getenv("USERPROFILE");
    if (!home) {
        std::cerr << "错误：无法确定 HOME 目录" << std::endl;
        std::exit(1);
    }
    return std::string(home) + "/.todo/todos.json";
}

int main(int argc, char* argv[]) {
    std::vector<std::string> args(argv + 1, argv + argc);

    std::string filepath = default_filepath();
    std::vector<std::string> cmd_args;

    // 解析全局选项
    size_t i = 0;
    while (i < args.size()) {
        if (args[i] == "--file") {
            if (i + 1 >= args.size()) {
                std::cerr << "错误：--file 需要参数" << std::endl;
                return 1;
            }
            filepath = args[i + 1];
            i += 2;
        } else {
            break;
        }
    }

    // 剩余参数
    while (i < args.size()) {
        cmd_args.push_back(args[i]);
        ++i;
    }

    if (cmd_args.empty()) {
        std::cerr << "错误：缺少命令。使用 'todo help' 查看帮助。" << std::endl;
        return 1;
    }

    const std::string& command = cmd_args[0];
    TodoStorage storage(filepath);

    if (command == "add") {
        if (cmd_args.size() < 2) {
            std::cerr << "错误：'add' 需要标题参数" << std::endl;
            return 1;
        }
        return storage.add(cmd_args[1]);
    } else if (command == "list") {
        return storage.list();
    } else if (command == "done") {
        if (cmd_args.size() < 2) {
            std::cerr << "错误：'done' 需要 ID 参数" << std::endl;
            return 1;
        }
        int id = std::stoi(cmd_args[1]);
        return storage.done(id);
    } else if (command == "delete") {
        if (cmd_args.size() < 2) {
            std::cerr << "错误：'delete' 需要 ID 参数" << std::endl;
            return 1;
        }
        int id = std::stoi(cmd_args[1]);
        return storage.remove(id);
    } else if (command == "edit") {
        if (cmd_args.size() < 3) {
            std::cerr << "错误：'edit' 需要 ID 和标题参数" << std::endl;
            return 1;
        }
        int id = std::stoi(cmd_args[1]);
        return storage.edit(id, cmd_args[2]);
    } else if (command == "clear") {
        return storage.clear_done();
    } else if (command == "help") {
        return storage.help();
    } else {
        std::cerr << "错误：未知命令 '" << command << "'。使用 'todo help' 查看帮助。" << std::endl;
        return 1;
    }
}
