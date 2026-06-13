import fs from "node:fs";
import path from "node:path";
import os from "node:os";

// ── 类型定义 ──────────────────────────────────────────────────────

interface TodoItem {
  id: number;
  title: string;
  completed: boolean;
  created_at: string;
  updated_at: string;
}

interface DataFile {
  todos: TodoItem[];
  next_id: number;
}

// ── 存储层 ────────────────────────────────────────────────────────

export class TodoStorage {
  private path: string;
  private items: TodoItem[];
  private nextId: number;

  constructor(filepath: string) {
    this.path = filepath;
    this.items = [];
    this.nextId = 1;
    this.load();
  }

  // ---- 文件 I/O ----

  private load(): void {
    try {
      const data = fs.readFileSync(this.path, "utf-8");
      const file: DataFile = JSON.parse(data);
      this.items = file.todos;
      this.nextId = file.next_id;
    } catch {
      // 文件不存在 → 空列表
    }
  }

  private save(): void {
    const dir = path.dirname(this.path);
    fs.mkdirSync(dir, { recursive: true });

    const file: DataFile = { todos: this.items, next_id: this.nextId };
    fs.writeFileSync(this.path, JSON.stringify(file, null, 2) + "\n");
  }

  private static now(): string {
    return new Date().toISOString().replace(/\.\d{3}Z$/, "Z");
  }

  private static fmtDisplayTime(iso: string): string {
    // "2025-01-01T10:00:00Z" → "2025-01-01 10:00:00 +0000"
    return iso.replace("T", " ").replace("Z", " +0000");
  }

  // ---- 命令实现 ----

  add(title: string): number {
    if (!title) {
      console.error("错误：标题不能为空");
      process.exit(1);
    }
    const nowStr = TodoStorage.now();
    const item: TodoItem = {
      id: this.nextId,
      title,
      completed: false,
      created_at: nowStr,
      updated_at: nowStr,
    };
    this.nextId++;
    console.log(`已添加：ID ${item.id} - ${item.title}`);
    this.items.push(item);
    this.save();
    return 0;
  }

  list(): number {
    if (this.items.length === 0) {
      console.log("暂无任务");
      return 0;
    }

    const sorted = [...this.items].sort((a, b) => a.id - b.id);

    console.log("ID  状态  标题                创建时间");
    console.log("─── ──── ──────────────────── ─────────────────────────");

    for (const item of sorted) {
      const status = item.completed ? "☑" : "☐";
      console.log(
        `${String(item.id).padStart(3)}  ${status.padEnd(4)} ${item.title.padEnd(20)}  ${TodoStorage.fmtDisplayTime(item.created_at)}`
      );
    }
    return 0;
  }

  done(id: number): number {
    const item = this.items.find((i) => i.id === id);
    if (!item) {
      console.error(`错误：ID ${id} 不存在`);
      return 1;
    }
    if (item.completed) {
      console.log(`任务 ID ${id} 已经是完成状态`);
      return 0;
    }
    item.completed = true;
    item.updated_at = TodoStorage.now();
    console.log(`已完成：ID ${item.id} - ${item.title}`);
    this.save();
    return 0;
  }

  delete(id: number): number {
    const idx = this.items.findIndex((i) => i.id === id);
    if (idx === -1) {
      console.error(`错误：ID ${id} 不存在`);
      return 1;
    }
    const item = this.items.splice(idx, 1)[0];
    console.log(`已删除：ID ${item.id} - ${item.title}`);
    this.save();
    return 0;
  }

  edit(id: number, newTitle: string): number {
    if (!newTitle) {
      console.error("错误：标题不能为空");
      process.exit(1);
    }
    const item = this.items.find((i) => i.id === id);
    if (!item) {
      console.error(`错误：ID ${id} 不存在`);
      return 1;
    }
    item.title = newTitle;
    item.updated_at = TodoStorage.now();
    console.log(`已更新：ID ${item.id} - ${item.title}`);
    this.save();
    return 0;
  }

  clearDone(): number {
    const before = this.items.length;
    this.items = this.items.filter((i) => !i.completed);
    const removed = before - this.items.length;
    if (removed === 0) {
      console.log("没有已完成的任务需要清除");
      return 0;
    }
    console.log(`已清除 ${removed} 条已完成任务`);
    this.save();
    return 0;
  }

  static help(): void {
    console.log("用法：todo [--file <路径>] <命令> [参数...]");
    console.log();
    console.log("命令：");
    console.log("  add <标题>        添加任务");
    console.log("  list              列出所有任务");
    console.log("  done <ID>         标记任务为已完成");
    console.log("  delete <ID>       删除任务");
    console.log("  edit <ID> <标题>  修改任务标题");
    console.log("  clear             清除所有已完成任务");
    console.log("  help              显示此帮助");
    console.log();
    console.log("全局选项：");
    console.log("  --file <路径>     指定数据文件路径（默认：~/.todo/todos.json）");
  }
}

// ── CLI 入口 ──────────────────────────────────────────────────────

function defaultFilepath(): string {
  const home = os.homedir();
  return path.join(home, ".todo", "todos.json");
}

export function main(): void {
  const args = process.argv.slice(2);

  let filepath = defaultFilepath();
  const cmdArgs: string[] = [];

  let i = 0;
  while (i < args.length) {
    if (args[i] === "--file") {
      if (i + 1 >= args.length) {
        console.error("错误：--file 需要参数");
        process.exit(1);
      }
      filepath = args[i + 1];
      i += 2;
    } else {
      break;
    }
  }

  while (i < args.length) {
    cmdArgs.push(args[i]);
    i++;
  }

  if (cmdArgs.length === 0) {
    console.error("错误：缺少命令。使用 'todo help' 查看帮助。");
    process.exit(1);
  }

  const command = cmdArgs[0];

  if (command === "help") {
    TodoStorage.help();
    return;
  }

  const storage = new TodoStorage(filepath);
  let exitCode = 0;

  switch (command) {
    case "add":
      if (cmdArgs.length < 2) {
        console.error("错误：'add' 需要标题参数");
        process.exit(1);
      }
      exitCode = storage.add(cmdArgs[1]);
      break;
    case "list":
      exitCode = storage.list();
      break;
    case "done":
      if (cmdArgs.length < 2) {
        console.error("错误：'done' 需要 ID 参数");
        process.exit(1);
      }
      exitCode = storage.done(Number(cmdArgs[1]));
      break;
    case "delete":
      if (cmdArgs.length < 2) {
        console.error("错误：'delete' 需要 ID 参数");
        process.exit(1);
      }
      exitCode = storage.delete(Number(cmdArgs[1]));
      break;
    case "edit":
      if (cmdArgs.length < 3) {
        console.error("错误：'edit' 需要 ID 和标题参数");
        process.exit(1);
      }
      exitCode = storage.edit(Number(cmdArgs[1]), cmdArgs[2]);
      break;
    case "clear":
      exitCode = storage.clearDone();
      break;
    default:
      console.error(`错误：未知命令 '${command}'。使用 'todo help' 查看帮助。`);
      process.exit(1);
  }

  process.exit(exitCode);
}

// 直接运行时
if (process.argv[1] && (process.argv[1].endsWith("index.ts") || process.argv[1].endsWith("index.js"))) {
  main();
}
