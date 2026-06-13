use chrono::Utc;
use serde::{Deserialize, Serialize};
use std::fs;
use std::io::{BufReader, BufWriter};
use std::path::PathBuf;

// ── 数据结构 ──────────────────────────────────────────────────────

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct TodoItem {
    pub id: u32,
    pub title: String,
    pub completed: bool,
    pub created_at: String,
    pub updated_at: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
struct DataFile {
    todos: Vec<TodoItem>,
    next_id: u32,
}

// ── 存储层 ────────────────────────────────────────────────────────

pub struct TodoStorage {
    path: PathBuf,
    items: Vec<TodoItem>,
    next_id: u32,
}

impl TodoStorage {
    pub fn new(path: PathBuf) -> Self {
        let mut s = Self {
            path,
            items: Vec::new(),
            next_id: 1,
        };
        s.load();
        s
    }

    // ---- 文件 I/O ----

    fn load(&mut self) {
        let file = match fs::File::open(&self.path) {
            Ok(f) => f,
            Err(_) => return, // 文件不存在 → 空列表
        };
        let reader = BufReader::new(file);
        let data: DataFile = match serde_json::from_reader(reader) {
            Ok(d) => d,
            Err(e) => {
                eprintln!("错误：JSON 解析失败 - {}", e);
                std::process::exit(1);
            }
        };
        self.items = data.todos;
        self.next_id = data.next_id;
    }

    fn save(&self) {
        // 确保目录存在
        if let Some(parent) = self.path.parent() {
            fs::create_dir_all(parent).ok();
        }

        let data = DataFile {
            todos: self.items.clone(),
            next_id: self.next_id,
        };
        let file = fs::File::create(&self.path).unwrap_or_else(|e| {
            eprintln!("错误：无法写入 {} - {}", self.path.display(), e);
            std::process::exit(1);
        });
        let writer = BufWriter::new(file);
        serde_json::to_writer_pretty(writer, &data).unwrap_or_else(|e| {
            eprintln!("错误：JSON 写入失败 - {}", e);
            std::process::exit(1);
        });
    }

    fn now() -> String {
        Utc::now().format("%Y-%m-%dT%H:%M:%SZ").to_string()
    }

    fn fmt_display_time(iso: &str) -> String {
        // "2025-01-01T10:00:00Z" → "2025-01-01 10:00:00 +0000"
        if iso.len() < 20 {
            return iso.to_string();
        }
        let mut s = iso.to_string();
        s.replace_range(10..11, " "); // T → 空格
        if s.ends_with('Z') {
            s.truncate(s.len() - 1);
            s.push_str(" +0000");
        }
        s
    }

    // ---- 命令实现 ----

    pub fn add(&mut self, title: &str) -> i32 {
        if title.is_empty() {
            eprintln!("错误：标题不能为空");
            std::process::exit(1);
        }
        let now = Self::now();
        let item = TodoItem {
            id: self.next_id,
            title: title.to_string(),
            completed: false,
            created_at: now.clone(),
            updated_at: now,
        };
        self.next_id += 1;
        println!("已添加：ID {} - {}", item.id, item.title);
        self.items.push(item);
        self.save();
        0
    }

    pub fn list(&self) -> i32 {
        if self.items.is_empty() {
            println!("暂无任务");
            return 0;
        }

        let mut sorted = self.items.clone();
        sorted.sort_by_key(|i| i.id);

        println!("ID  状态  标题                创建时间");
        println!("─── ──── ──────────────────── ─────────────────────────");

        for item in &sorted {
            let status = if item.completed { "☑" } else { "☐" };
            println!(
                "{:>3}  {:<4} {:<20}  {}",
                item.id,
                status,
                item.title,
                Self::fmt_display_time(&item.created_at)
            );
        }
        0
    }

    pub fn done(&mut self, id: u32) -> i32 {
        let item = match self.items.iter_mut().find(|i| i.id == id) {
            Some(i) => i,
            None => {
                eprintln!("错误：ID {} 不存在", id);
                return 1;
            }
        };
        if item.completed {
            println!("任务 ID {} 已经是完成状态", id);
            return 0;
        }
        item.completed = true;
        item.updated_at = Self::now();
        println!("已完成：ID {} - {}", item.id, item.title);
        self.save();
        0
    }

    pub fn delete(&mut self, id: u32) -> i32 {
        let pos = match self.items.iter().position(|i| i.id == id) {
            Some(p) => p,
            None => {
                eprintln!("错误：ID {} 不存在", id);
                return 1;
            }
        };
        let item = self.items.remove(pos);
        println!("已删除：ID {} - {}", item.id, item.title);
        self.save();
        0
    }

    pub fn edit(&mut self, id: u32, new_title: &str) -> i32 {
        if new_title.is_empty() {
            eprintln!("错误：标题不能为空");
            std::process::exit(1);
        }
        let item = match self.items.iter_mut().find(|i| i.id == id) {
            Some(i) => i,
            None => {
                eprintln!("错误：ID {} 不存在", id);
                return 1;
            }
        };
        item.title = new_title.to_string();
        item.updated_at = Self::now();
        println!("已更新：ID {} - {}", item.id, item.title);
        self.save();
        0
    }

    pub fn clear_done(&mut self) -> i32 {
        let before = self.items.len();
        self.items.retain(|i| !i.completed);
        let removed = before - self.items.len();
        if removed == 0 {
            println!("没有已完成的任务需要清除");
            return 0;
        }
        println!("已清除 {} 条已完成任务", removed);
        self.save();
        0
    }

    pub fn help() -> i32 {
        println!("用法：todo [--file <路径>] <命令> [参数...]");
        println!();
        println!("命令：");
        println!("  add <标题>        添加任务");
        println!("  list              列出所有任务");
        println!("  done <ID>         标记任务为已完成");
        println!("  delete <ID>       删除任务");
        println!("  edit <ID> <标题>  修改任务标题");
        println!("  clear             清除所有已完成任务");
        println!("  help              显示此帮助");
        println!();
        println!("全局选项：");
        println!("  --file <路径>     指定数据文件路径（默认：~/.todo/todos.json）");
        0
    }
}

// ── 测试 ──────────────────────────────────────────────────────────

#[cfg(test)]
mod tests {
    use super::*;

    fn temp_path(name: &str) -> PathBuf {
        let mut p = std::env::temp_dir();
        p.push(format!(
            "todo_test_{}_{}.json",
            std::process::id(),
            name
        ));
        // 如果已存在，先删除
        let _ = fs::remove_file(&p);
        p
    }

    fn with_storage<F>(name: &str, f: F)
    where
        F: FnOnce(&mut TodoStorage),
    {
        let path = temp_path(name);
        let mut s = TodoStorage::new(path.clone());
        f(&mut s);
        let _ = fs::remove_file(&path);
    }

    #[test]
    fn test_add_todo() {
        with_storage("add_todo", |s| {
            assert_eq!(s.add("测试任务1"), 0);
            assert_eq!(s.items.len(), 1);
            assert_eq!(s.items[0].title, "测试任务1");
        });
    }

    #[test]
    fn test_list_empty() {
        let path = temp_path("list_empty");
        let s = TodoStorage::new(path.clone());
        // list() 只是打印，不 panic 即可
        assert_eq!(s.list(), 0);
        let _ = fs::remove_file(&path);
    }

    #[test]
    fn test_done() {
        with_storage("done", |s| {
            s.add("可完成");
            assert_eq!(s.done(1), 0);
            assert!(s.items[0].completed);
        });
    }

    #[test]
    fn test_done_already_completed() {
        with_storage("done_already", |s| {
            s.add("已完成");
            s.done(1);
            assert_eq!(s.done(1), 0); // 幂等，不报错
            assert!(s.items[0].completed);
        });
    }

    #[test]
    fn test_delete() {
        with_storage("delete", |s| {
            s.add("待删除");
            assert_eq!(s.delete(1), 0);
            assert!(s.items.is_empty());
        });
    }

    #[test]
    fn test_edit() {
        with_storage("edit", |s| {
            s.add("旧标题");
            assert_eq!(s.edit(1, "新标题"), 0);
            assert_eq!(s.items[0].title, "新标题");
        });
    }

    #[test]
    fn test_id_not_found() {
        with_storage("id_not_found", |s| {
            assert_eq!(s.done(999), 1);
            assert_eq!(s.delete(999), 1);
            assert_eq!(s.edit(999, "任意"), 1);
        });
    }

    #[test]
    fn test_clear_done() {
        with_storage("clear_done", |s| {
            s.add("任务 A");
            s.add("任务 B");
            s.done(2);
            assert_eq!(s.clear_done(), 0);
            assert_eq!(s.items.len(), 1);
            assert_eq!(s.items[0].id, 1);
        });
    }

    #[test]
    fn test_clear_idempotent() {
        with_storage("clear_idempotent", |s| {
            s.add("只有未完成");
            assert_eq!(s.clear_done(), 0);
            assert_eq!(s.items.len(), 1);
        });
    }

    #[test]
    fn test_persistence() {
        let path = temp_path("persistence");
        {
            let mut s = TodoStorage::new(path.clone());
            s.add("持久化测试");
        }
        {
            let s = TodoStorage::new(path.clone());
            assert_eq!(s.items.len(), 1);
            assert_eq!(s.items[0].title, "持久化测试");
        }
        let _ = fs::remove_file(&path);
    }

    #[test]
    fn test_file_not_exists() {
        let path = PathBuf::from("/tmp/todo_test_nonexistent_ok.json");
        let s = TodoStorage::new(path.clone());
        assert_eq!(s.list(), 0);
        let _ = fs::remove_file(&path);
    }
}
