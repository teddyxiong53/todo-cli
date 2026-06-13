#include "todo.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/stat.h>

namespace fs = std::filesystem;

// ── 工具函数 ──────────────────────────────────────────────────────

std::string TodoStorage::timestamp_now() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm gmt = *std::gmtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&gmt, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::string TodoStorage::format_display_time(const std::string& iso) {
    // "2025-01-01T10:00:00Z" → "2025-01-01 10:00:00 +0000"
    if (iso.size() < 20) return iso;
    std::string s = iso;
    s[10] = ' ';                 // T → 空格
    // 去掉尾部的 Z，替换成 " +0000"
    if (s.back() == 'Z') {
        s.back() = ' ';
        s += "+0000";
    }
    return s;
}

void TodoStorage::ensure_dir(const std::string& dir) {
    if (dir.empty()) return;
    fs::path p(dir);
    if (!fs::exists(p)) {
        fs::create_directories(p);
    }
}

// ── 构造 / 加载 / 保存 ────────────────────────────────────────────

TodoStorage::TodoStorage(const std::string& filepath) : filepath_(filepath) {
    load();
}

void TodoStorage::load() {
    items_.clear();
    next_id_ = 1;

    std::ifstream ifs(filepath_);
    if (!ifs.is_open()) {
        // 文件不存在 → 空列表
        return;
    }

    try {
        json j;
        ifs >> j;

        if (j.contains("todos")) {
            items_ = j["todos"].get<std::vector<TodoItem>>();
        }
        if (j.contains("next_id")) {
            next_id_ = j["next_id"].get<int>();
        }
    } catch (const json::exception& e) {
        std::cerr << "错误：JSON 解析失败 - " << e.what() << std::endl;
        std::exit(1);
    }
}

void TodoStorage::save() {
    ensure_dir(fs::path(filepath_).parent_path().string());

    json j;
    j["todos"] = items_;
    j["next_id"] = next_id_;

    std::ofstream ofs(filepath_);
    if (!ofs.is_open()) {
        std::cerr << "错误：无法写入 " << filepath_ << std::endl;
        std::exit(1);
    }
    ofs << j.dump(2) << std::endl;
}

// ── 命令实现 ──────────────────────────────────────────────────────

int TodoStorage::add(const std::string& title) {
    if (title.empty()) {
        std::cerr << "错误：标题不能为空" << std::endl;
        std::exit(1);
    }

    TodoItem item;
    item.id = next_id_++;
    item.title = title;
    item.completed = false;
    item.created_at = timestamp_now();
    item.updated_at = item.created_at;

    items_.push_back(item);
    save();

    std::cout << "已添加：ID " << item.id << " - " << item.title << std::endl;
    return 0;
}

int TodoStorage::list() {
    if (items_.empty()) {
        std::cout << "暂无任务" << std::endl;
        return 0;
    }

    // 按 id 升序排列（通常已经有序，但保险）
    std::sort(items_.begin(), items_.end(),
              [](const TodoItem& a, const TodoItem& b) { return a.id < b.id; });

    // 表头
    std::cout << "ID  状态  标题                创建时间" << std::endl;
    std::cout << "─── ──── ──────────────────── ─────────────────────────" << std::endl;

    for (const auto& item : items_) {
        const char* status = item.completed ? "☑" : "☐";
        std::cout << std::setw(3) << std::right << item.id << "  "
                  << status << "    "
                  << std::setw(20) << std::left << item.title << "  "
                  << format_display_time(item.created_at)
                  << std::endl;
    }

    return 0;
}

int TodoStorage::done(int id) {
    auto it = std::find_if(items_.begin(), items_.end(),
                           [id](const TodoItem& item) { return item.id == id; });

    if (it == items_.end()) {
        std::cerr << "错误：ID " << id << " 不存在" << std::endl;
        return 1;
    }

    if (it->completed) {
        std::cout << "任务 ID " << id << " 已经是完成状态" << std::endl;
        return 0;
    }

    it->completed = true;
    it->updated_at = timestamp_now();
    save();

    std::cout << "已完成：ID " << it->id << " - " << it->title << std::endl;
    return 0;
}

int TodoStorage::remove(int id) {
    auto it = std::find_if(items_.begin(), items_.end(),
                           [id](const TodoItem& item) { return item.id == id; });

    if (it == items_.end()) {
        std::cerr << "错误：ID " << id << " 不存在" << std::endl;
        return 1;
    }

    std::string title = it->title;
    items_.erase(it);
    save();

    std::cout << "已删除：ID " << id << " - " << title << std::endl;
    return 0;
}

int TodoStorage::edit(int id, const std::string& new_title) {
    if (new_title.empty()) {
        std::cerr << "错误：标题不能为空" << std::endl;
        std::exit(1);
    }

    auto it = std::find_if(items_.begin(), items_.end(),
                           [id](const TodoItem& item) { return item.id == id; });

    if (it == items_.end()) {
        std::cerr << "错误：ID " << id << " 不存在" << std::endl;
        return 1;
    }

    it->title = new_title;
    it->updated_at = timestamp_now();
    save();

    std::cout << "已更新：ID " << it->id << " - " << it->title << std::endl;
    return 0;
}

int TodoStorage::clear_done() {
    auto count_before = items_.size();
    items_.erase(std::remove_if(items_.begin(), items_.end(),
                                [](const TodoItem& item) { return item.completed; }),
                 items_.end());

    size_t removed = count_before - items_.size();
    if (removed == 0) {
        std::cout << "没有已完成的任务需要清除" << std::endl;
        return 0;
    }

    save();
    std::cout << "已清除 " << removed << " 条已完成任务" << std::endl;
    return 0;
}

int TodoStorage::help() {
    std::cout << "用法：todo [--file <路径>] <命令> [参数...]" << std::endl;
    std::cout << std::endl;
    std::cout << "命令：" << std::endl;
    std::cout << "  add <标题>        添加任务" << std::endl;
    std::cout << "  list              列出所有任务" << std::endl;
    std::cout << "  done <ID>         标记任务为已完成" << std::endl;
    std::cout << "  delete <ID>       删除任务" << std::endl;
    std::cout << "  edit <ID> <标题>  修改任务标题" << std::endl;
    std::cout << "  clear             清除所有已完成任务" << std::endl;
    std::cout << "  help              显示此帮助" << std::endl;
    std::cout << std::endl;
    std::cout << "全局选项：" << std::endl;
    std::cout << "  --file <路径>     指定数据文件路径（默认：~/.todo/todos.json）" << std::endl;
    return 0;
}
