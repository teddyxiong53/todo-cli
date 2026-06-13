package main

import (
	"fmt"
	"os"
	"strconv"
)

func defaultFilepath() string {
	home, err := os.UserHomeDir()
	if err != nil {
		fmt.Fprintln(os.Stderr, "错误：无法确定 HOME 目录")
		os.Exit(1)
	}
	return home + "/.todo/todos.json"
}

func main() {
	args := os.Args[1:]

	filepath := defaultFilepath()
	var cmdArgs []string

	// 解析全局选项
	i := 0
	for i < len(args) {
		if args[i] == "--file" {
			if i+1 >= len(args) {
				fmt.Fprintln(os.Stderr, "错误：--file 需要参数")
				os.Exit(1)
			}
			filepath = args[i+1]
			i += 2
		} else {
			break
		}
	}

	cmdArgs = args[i:]

	if len(cmdArgs) == 0 {
		fmt.Fprintln(os.Stderr, "错误：缺少命令。使用 'todo help' 查看帮助。")
		os.Exit(1)
	}

	command := cmdArgs[0]

	if command == "help" {
		Help()
		return
	}

	// 需要存储的命令
	storage := NewTodoStorage(filepath)

	switch command {
	case "add":
		if len(cmdArgs) < 2 {
			fmt.Fprintln(os.Stderr, "错误：'add' 需要标题参数")
			os.Exit(1)
		}
		if err := storage.Add(cmdArgs[1]); err != nil {
			fmt.Fprintf(os.Stderr, "错误：%v\n", err)
			os.Exit(1)
		}
	case "list":
		if err := storage.List(); err != nil {
			fmt.Fprintf(os.Stderr, "错误：%v\n", err)
			os.Exit(1)
		}
	case "done":
		if len(cmdArgs) < 2 {
			fmt.Fprintln(os.Stderr, "错误：'done' 需要 ID 参数")
			os.Exit(1)
		}
		id, err := strconv.Atoi(cmdArgs[1])
		if err != nil {
			fmt.Fprintln(os.Stderr, "错误：ID 必须是数字")
			os.Exit(1)
		}
		if err := storage.Done(id); err != nil {
			fmt.Fprintf(os.Stderr, "错误：%v\n", err)
			os.Exit(1)
		}
	case "delete":
		if len(cmdArgs) < 2 {
			fmt.Fprintln(os.Stderr, "错误：'delete' 需要 ID 参数")
			os.Exit(1)
		}
		id, err := strconv.Atoi(cmdArgs[1])
		if err != nil {
			fmt.Fprintln(os.Stderr, "错误：ID 必须是数字")
			os.Exit(1)
		}
		if err := storage.Delete(id); err != nil {
			fmt.Fprintf(os.Stderr, "错误：%v\n", err)
			os.Exit(1)
		}
	case "edit":
		if len(cmdArgs) < 3 {
			fmt.Fprintln(os.Stderr, "错误：'edit' 需要 ID 和标题参数")
			os.Exit(1)
		}
		id, err := strconv.Atoi(cmdArgs[1])
		if err != nil {
			fmt.Fprintln(os.Stderr, "错误：ID 必须是数字")
			os.Exit(1)
		}
		if err := storage.Edit(id, cmdArgs[2]); err != nil {
			fmt.Fprintf(os.Stderr, "错误：%v\n", err)
			os.Exit(1)
		}
	case "clear":
		if err := storage.ClearDone(); err != nil {
			fmt.Fprintf(os.Stderr, "错误：%v\n", err)
			os.Exit(1)
		}
	default:
		fmt.Fprintf(os.Stderr, "错误：未知命令 '%s'。使用 'todo help' 查看帮助。\n", command)
		os.Exit(1)
	}
}
