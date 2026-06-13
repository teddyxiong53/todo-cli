# Todo CLI — 多语言实现规格说明

## 1. 概述

一个命令行 TODO 任务管理工具。支持添加、查看、完成、删除、编辑任务。通过实现同一份规格来对比不同编程语言的项目结构、构建方式、生态习惯。

## 2. 数据存储

### 2.1 存储文件

- 默认路径：`~/.todo/todos.json`
- 可通过命令行参数 `--file <path>` 覆盖

### 2.2 JSON 格式

```json
{
  "todos": [
    {
      "id": 1,
      "title": "学习 Rust",
      "completed": false,
      "created_at": "2025-01-01T10:00:00Z",
      "updated_at": "2025-01-01T10:00:00Z"
    }
  ],
  "next_id": 2
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | 整数 | 自增唯一 ID |
| `title` | 字符串 | 任务描述 |
| `completed` | 布尔 | 是否已完成 |
| `created_at` | 字符串 (ISO 8601) | 创建时间 |
| `updated_at` | 字符串 (ISO 8601) | 最后修改时间 |
| `next_id` | 整数 | 下一个可用 ID |

### 2.3 并发安全

- 单用户使用，不考虑并发写入。
- 每次操作读取 → 修改 → 写入整个文件。

### 2.4 目录创建

- 如果 `~/.todo/` 目录不存在，首次使用时自动创建。
- 如果 `~/.todo/todos.json` 文件不存在，视为空列表。

## 3. CLI 接口

### 3.1 全局选项

| 选项 | 说明 | 默认值 |
|------|------|--------|
| `--file <path>` | 指定数据文件路径 | `~/.todo/todos.json` |

### 3.2 子命令

#### `add <title>`
- 添加一条任务。
- `<title>` 必须提供，不可为空。
- 自动生成 `id`、`created_at`、`updated_at`，`completed` 初始化为 `false`。
- 输出：`已添加：ID {id} - {title}`

#### `list`
- 列出所有任务。按 `id` 升序排列。
- 输出表格：
  - 列：ID、状态（☐ 未完成 / ☑ 已完成）、标题、创建时间
  - 如果没有任务，输出：`暂无任务`

#### `done <id>`
- 将指定 ID 的任务标记为已完成。
- 更新 `completed = true`、`updated_at`。
- 输出：`已完成：ID {id} - {title}`
- 如果 ID 不存在，输出错误并退出码 1。
- 如果任务已经完成，输出：`任务 ID {id} 已经是完成状态`

#### `delete <id>`
- 删除指定 ID 的任务。
- 输出：`已删除：ID {id} - {title}`
- 如果 ID 不存在，输出错误并退出码 1。

#### `edit <id> <new-title>`
- 修改指定 ID 的任务标题。
- `<new-title>` 不可为空。
- 更新 `title`、`updated_at`。
- 输出：`已更新：ID {id} - {title}`
- 如果 ID 不存在，输出错误并退出码 1。

#### `clear`
- 删除所有已完成的任务。
- 输出：`已清除 {n} 条已完成任务`
- 如果没有已完成的任务，输出：`没有已完成的任务需要清除`

#### `help`
- 打印使用帮助。

### 3.3 退出码

| 退出码 | 说明 |
|--------|------|
| 0 | 成功 |
| 1 | 一般错误（ID 不存在、参数缺失等） |

### 3.4 输出规范

- 正常情况下所有输出写至 `stdout`。
- 错误消息输出到 `stderr`。
- List 表格格式（示例）：

```
ID  状态  标题                创建时间
─── ──── ──────────────────── ─────────────────────────
 1  ☐    学习 Rust            2025-01-01 10:00:00 +0000
 2  ☑    学习 Go              2025-01-02 14:30:00 +0000
```

- 各语言实现时表格须对齐，使用等宽字体友好。

## 4. 构建与运行

### 4.1 通用 Makefile 目标

所有语言在顶层 Makefile 中暴露以下目标：

| 目标 | 说明 |
|------|------|
| `build` | 编译/构建 |
| `run` | 运行（可传递参数：`make run ARGS="list"`） |
| `test` | 运行该语言的测试 |
| `clean` | 清理构建产物 |

### 4.2 各语言构建方式

| 语言 | 构建系统 | 可执行文件 |
|------|----------|-----------|
| C++  | CMake | `cpp/build/todo` |
| Rust | Cargo | `rust/target/release/todo` |
| Go   | go build | `go/todo` |
| TS   | Node.js / tsx | `ts/src/index.ts` (npx tsx) |
| Python | 直接运行 | `python/todo_cli/__main__.py` (python3 -m) |
| GLib (C) | Makefile + pkg-config | `glib/todo` |
| Shell | 直接运行 | `shell/todo.sh` |
| Lua  | 直接运行 | `lua/todo_cli.lua` (lua) |

## 5. 测试策略

- 每个语言应使用该语言主流的测试框架编写单元测试。
- 至少覆盖：
  - 添加任务
  - 列表为空
  - 标记完成
  - 标记已完成的任务再次完成（幂等）
  - 删除任务
  - 编辑任务
  - ID 不存在时返回错误
  - clear 清除已完成任务
  - clear 没有已完成任务（幂等）
  - 存储文件不存在时的初始化

## 6. 各语言约束

### 6.1 C++
- 使用 CMake 构建。
- 依赖：nlohmann/json（使用 FetchContent 自动拉取）。
- C++17 标准。

### 6.2 Rust
- 使用 Cargo 构建。
- 依赖：serde / serde_json。
- 使用 clap 解析命令行参数（可选，也可手动解析）。

### 6.3 Go
- Go Modules 管理依赖。
- 依赖：标准库即可（无需第三方 json 库）。

### 6.4 TypeScript
- 使用 pnpm 或 npm 管理依赖。
- 使用 tsx 直接运行 TypeScript（或先编译再运行）。
- 依赖：无需第三方 CLI 库。

### 6.5 Python
- Python 3.10+。
- 标准库即可（json, argparse）。

### 6.6 Shell
- Bash 4+。
- 依赖：jq（JSON 处理）。
- 不依赖其他语言运行时。
