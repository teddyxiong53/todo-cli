"""CLI argument parsing and dispatch."""

import os
import sys
from pathlib import Path

from .storage import TodoStorage


def default_filepath() -> str:
    home = os.path.expanduser("~")
    return str(Path(home) / ".todo" / "todos.json")


def main() -> None:
    args = sys.argv[1:]

    filepath = default_filepath()
    cmd_args: list[str] = []

    # 解析全局选项
    i = 0
    while i < len(args):
        if args[i] == "--file":
            if i + 1 >= len(args):
                print("错误：--file 需要参数", file=sys.stderr)
                sys.exit(1)
            filepath = args[i + 1]
            i += 2
        else:
            break

    cmd_args = args[i:]

    if not cmd_args:
        print("错误：缺少命令。使用 'todo help' 查看帮助。", file=sys.stderr)
        sys.exit(1)

    command = cmd_args[0]

    if command == "help":
        TodoStorage.help()
        return

    storage = TodoStorage(filepath)

    if command == "add":
        if len(cmd_args) < 2:
            print("错误：'add' 需要标题参数", file=sys.stderr)
            sys.exit(1)
        exit_code = storage.add(cmd_args[1])
    elif command == "list":
        exit_code = storage.list()
    elif command == "done":
        if len(cmd_args) < 2:
            print("错误：'done' 需要 ID 参数", file=sys.stderr)
            sys.exit(1)
        try:
            id_val = int(cmd_args[1])
        except ValueError:
            print("错误：ID 必须是数字", file=sys.stderr)
            sys.exit(1)
        exit_code = storage.done(id_val)
    elif command == "delete":
        if len(cmd_args) < 2:
            print("错误：'delete' 需要 ID 参数", file=sys.stderr)
            sys.exit(1)
        try:
            id_val = int(cmd_args[1])
        except ValueError:
            print("错误：ID 必须是数字", file=sys.stderr)
            sys.exit(1)
        exit_code = storage.delete(id_val)
    elif command == "edit":
        if len(cmd_args) < 3:
            print("错误：'edit' 需要 ID 和标题参数", file=sys.stderr)
            sys.exit(1)
        try:
            id_val = int(cmd_args[1])
        except ValueError:
            print("错误：ID 必须是数字", file=sys.stderr)
            sys.exit(1)
        exit_code = storage.edit(id_val, cmd_args[2])
    elif command == "clear":
        exit_code = storage.clear_done()
    else:
        print(f"错误：未知命令 '{command}'。使用 'todo help' 查看帮助。", file=sys.stderr)
        sys.exit(1)

    sys.exit(exit_code)
