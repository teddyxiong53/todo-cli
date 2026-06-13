mod storage;

use std::env;
use std::path::PathBuf;
use storage::TodoStorage;

fn default_filepath() -> PathBuf {
    let home = env::var("HOME")
        .or_else(|_| env::var("USERPROFILE"))
        .unwrap_or_else(|_| {
            eprintln!("错误：无法确定 HOME 目录");
            std::process::exit(1);
        });
    PathBuf::from(home).join(".todo/todos.json")
}

fn main() {
    let args: Vec<String> = env::args().collect();
    let mut filepath = default_filepath();

    // 解析全局选项
    let mut i = 1;
    while i < args.len() {
        if args[i] == "--file" {
            if i + 1 >= args.len() {
                eprintln!("错误：--file 需要参数");
                std::process::exit(1);
            }
            filepath = PathBuf::from(&args[i + 1]);
            i += 2;
        } else {
            break;
        }
    }

    // 剩余命令参数
    let cmd_args: Vec<&str> = args[i..].iter().map(String::as_str).collect();

    if cmd_args.is_empty() {
        eprintln!("错误：缺少命令。使用 'todo help' 查看帮助。");
        std::process::exit(1);
    }

    let command = cmd_args[0];

    if command == "help" {
        TodoStorage::help();
        return;
    }

    // 需要存储的命令
    let mut storage = TodoStorage::new(filepath);

    let exit_code = match command {
        "add" => {
            if cmd_args.len() < 2 {
                eprintln!("错误：'add' 需要标题参数");
                std::process::exit(1);
            }
            storage.add(cmd_args[1])
        }
        "list" => storage.list(),
        "done" => {
            if cmd_args.len() < 2 {
                eprintln!("错误：'done' 需要 ID 参数");
                std::process::exit(1);
            }
            let id: u32 = cmd_args[1].parse().unwrap_or_else(|_| {
                eprintln!("错误：ID 必须是数字");
                std::process::exit(1);
            });
            storage.done(id)
        }
        "delete" => {
            if cmd_args.len() < 2 {
                eprintln!("错误：'delete' 需要 ID 参数");
                std::process::exit(1);
            }
            let id: u32 = cmd_args[1].parse().unwrap_or_else(|_| {
                eprintln!("错误：ID 必须是数字");
                std::process::exit(1);
            });
            storage.delete(id)
        }
        "edit" => {
            if cmd_args.len() < 3 {
                eprintln!("错误：'edit' 需要 ID 和标题参数");
                std::process::exit(1);
            }
            let id: u32 = cmd_args[1].parse().unwrap_or_else(|_| {
                eprintln!("错误：ID 必须是数字");
                std::process::exit(1);
            });
            storage.edit(id, cmd_args[2])
        }
        "clear" => storage.clear_done(),
        other => {
            eprintln!("错误：未知命令 '{}'。使用 'todo help' 查看帮助。", other);
            std::process::exit(1);
        }
    };

    std::process::exit(exit_code);
}
