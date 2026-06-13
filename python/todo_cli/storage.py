"""Data persistence layer."""

from __future__ import annotations

import json
import sys
from dataclasses import asdict, dataclass
from datetime import datetime, timezone
from pathlib import Path


@dataclass
class TodoItem:
    id: int
    title: str
    completed: bool = False
    created_at: str = ""
    updated_at: str = ""


class TodoStorage:
    def __init__(self, filepath: str) -> None:
        self.path = Path(filepath)
        self.items: list[TodoItem] = []
        self.next_id = 1
        self._load()

    # ---- 文件 I/O ----

    def _load(self) -> None:
        if not self.path.exists():
            return
        try:
            with open(self.path, "r", encoding="utf-8") as f:
                data = json.load(f)
            self.items = [TodoItem(**item) for item in data["todos"]]
            self.next_id = data["next_id"]
        except (json.JSONDecodeError, KeyError) as e:
            print(f"错误：JSON 解析失败 - {e}", file=sys.stderr)
            sys.exit(1)

    def _save(self) -> None:
        self.path.parent.mkdir(parents=True, exist_ok=True)
        data = {
            "todos": [asdict(item) for item in self.items],
            "next_id": self.next_id,
        }
        with open(self.path, "w", encoding="utf-8") as f:
            json.dump(data, f, ensure_ascii=False, indent=2)
            f.write("\n")

    @staticmethod
    def _now() -> str:
        return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

    @staticmethod
    def _fmt_display_time(iso: str) -> str:
        return iso.replace("T", " ").replace("Z", " +0000")

    # ---- 命令实现 ----

    def add(self, title: str) -> int:
        if not title:
            print("错误：标题不能为空", file=sys.stderr)
            sys.exit(1)
        now = self._now()
        item = TodoItem(
            id=self.next_id,
            title=title,
            completed=False,
            created_at=now,
            updated_at=now,
        )
        self.next_id += 1
        print(f"已添加：ID {item.id} - {item.title}")
        self.items.append(item)
        self._save()
        return 0

    def list(self) -> int:
        if not self.items:
            print("暂无任务")
            return 0

        sorted_items = sorted(self.items, key=lambda i: i.id)

        print("ID  状态  标题                创建时间")
        print("─── ──── ──────────────────── ─────────────────────────")

        for item in sorted_items:
            status = "☑" if item.completed else "☐"
            print(
                f"{item.id:>3}  {status:<4} {item.title:<20}  {self._fmt_display_time(item.created_at)}"
            )
        return 0

    def done(self, id: int) -> int:
        for item in self.items:
            if item.id == id:
                if item.completed:
                    print(f"任务 ID {id} 已经是完成状态")
                    return 0
                item.completed = True
                item.updated_at = self._now()
                print(f"已完成：ID {item.id} - {item.title}")
                self._save()
                return 0
        print(f"错误：ID {id} 不存在", file=sys.stderr)
        return 1

    def delete(self, id: int) -> int:
        for i, item in enumerate(self.items):
            if item.id == id:
                self.items.pop(i)
                print(f"已删除：ID {item.id} - {item.title}")
                self._save()
                return 0
        print(f"错误：ID {id} 不存在", file=sys.stderr)
        return 1

    def edit(self, id: int, new_title: str) -> int:
        if not new_title:
            print("错误：标题不能为空", file=sys.stderr)
            sys.exit(1)
        for item in self.items:
            if item.id == id:
                item.title = new_title
                item.updated_at = self._now()
                print(f"已更新：ID {item.id} - {item.title}")
                self._save()
                return 0
        print(f"错误：ID {id} 不存在", file=sys.stderr)
        return 1

    def clear_done(self) -> int:
        before = len(self.items)
        self.items = [i for i in self.items if not i.completed]
        removed = before - len(self.items)
        if removed == 0:
            print("没有已完成的任务需要清除")
            return 0
        print(f"已清除 {removed} 条已完成任务")
        self._save()
        return 0

    @staticmethod
    def help() -> None:
        print("用法：todo [--file <路径>] <命令> [参数...]")
        print()
        print("命令：")
        print("  add <标题>        添加任务")
        print("  list              列出所有任务")
        print("  done <ID>         标记任务为已完成")
        print("  delete <ID>       删除任务")
        print("  edit <ID> <标题>  修改任务标题")
        print("  clear             清除所有已完成任务")
        print("  help              显示此帮助")
        print()
        print("全局选项：")
        print("  --file <路径>     指定数据文件路径（默认：~/.todo/todos.json）")
