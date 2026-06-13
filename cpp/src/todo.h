#ifndef TODO_H
#define TODO_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct TodoItem {
    int id = 0;
    std::string title;
    bool completed = false;
    std::string created_at;
    std::string updated_at;
};

class TodoStorage {
public:
    explicit TodoStorage(const std::string& filepath);

    // 命令接口：返回 0 成功，非 0 错误
    int add(const std::string& title);
    int list();
    int done(int id);
    int remove(int id);
    int edit(int id, const std::string& new_title);
    int clear_done();
    int help();

    static std::string timestamp_now();

private:
    // 加载 / 保存
    void load();
    void save();

    // 确保目录存在
    static void ensure_dir(const std::string& dir);

    // 格式化时间用于显示（"2025-01-01 10:00:00 +0000"）
    static std::string format_display_time(const std::string& iso);

    std::string filepath_;
    std::vector<TodoItem> items_;
    int next_id_ = 1;
};

// JSON 序列化
inline void to_json(json& j, const TodoItem& t) {
    j = json{
        {"id", t.id},
        {"title", t.title},
        {"completed", t.completed},
        {"created_at", t.created_at},
        {"updated_at", t.updated_at}
    };
}

inline void from_json(const json& j, TodoItem& t) {
    j.at("id").get_to(t.id);
    j.at("title").get_to(t.title);
    j.at("completed").get_to(t.completed);
    j.at("created_at").get_to(t.created_at);
    j.at("updated_at").get_to(t.updated_at);
}

#endif // TODO_H
