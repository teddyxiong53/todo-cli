package main

import (
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"time"
)

// ── 数据结构 ──────────────────────────────────────────────────────

type TodoItem struct {
	ID        int    `json:"id"`
	Title     string `json:"title"`
	Completed bool   `json:"completed"`
	CreatedAt string `json:"created_at"`
	UpdatedAt string `json:"updated_at"`
}

type DataFile struct {
	Todos  []TodoItem `json:"todos"`
	NextID int        `json:"next_id"`
}

// ── 存储层 ────────────────────────────────────────────────────────

type TodoStorage struct {
	path   string
	items  []TodoItem
	nextID int
}

func NewTodoStorage(path string) *TodoStorage {
	s := &TodoStorage{
		path:   path,
		items:  make([]TodoItem, 0),
		nextID: 1,
	}
	s.load()
	return s
}

// ---- 文件 I/O ----

func (s *TodoStorage) load() {
	data, err := os.ReadFile(s.path)
	if err != nil {
		// 文件不存在 → 空列表
		return
	}
	var file DataFile
	if err := json.Unmarshal(data, &file); err != nil {
		fmt.Fprintf(os.Stderr, "错误：JSON 解析失败 - %v\n", err)
		os.Exit(1)
	}
	s.items = file.Todos
	s.nextID = file.NextID
}

func (s *TodoStorage) save() {
	// 确保目录存在
	dir := filepath.Dir(s.path)
	os.MkdirAll(dir, 0755)

	file := DataFile{
		Todos:  s.items,
		NextID: s.nextID,
	}
	data, err := json.MarshalIndent(file, "", "  ")
	if err != nil {
		fmt.Fprintf(os.Stderr, "错误：JSON 序列化失败 - %v\n", err)
		os.Exit(1)
	}
	if err := os.WriteFile(s.path, data, 0644); err != nil {
		fmt.Fprintf(os.Stderr, "错误：无法写入 %s - %v\n", s.path, err)
		os.Exit(1)
	}
}

func now() string {
	return time.Now().UTC().Format("2006-01-02T15:04:05Z")
}

func fmtDisplayTime(iso string) string {
	// "2025-01-01T10:00:00Z" → "2025-01-01 10:00:00 +0000"
	if len(iso) < 20 {
		return iso
	}
	s := strings.Replace(iso, "T", " ", 1)
	if strings.HasSuffix(s, "Z") {
		s = s[:len(s)-1] + " +0000"
	}
	return s
}

// ---- 命令实现 ----

func (s *TodoStorage) Add(title string) error {
	if title == "" {
		return fmt.Errorf("标题不能为空")
	}
	nowStr := now()
	item := TodoItem{
		ID:        s.nextID,
		Title:     title,
		Completed: false,
		CreatedAt: nowStr,
		UpdatedAt: nowStr,
	}
	s.nextID++
	fmt.Printf("已添加：ID %d - %s\n", item.ID, item.Title)
	s.items = append(s.items, item)
	s.save()
	return nil
}

func (s *TodoStorage) List() error {
	if len(s.items) == 0 {
		fmt.Println("暂无任务")
		return nil
	}

	// 按 id 升序
	sorted := make([]TodoItem, len(s.items))
	copy(sorted, s.items)
	sort.Slice(sorted, func(i, j int) bool { return sorted[i].ID < sorted[j].ID })

	fmt.Println("ID  状态  标题                创建时间")
	fmt.Println("─── ──── ──────────────────── ─────────────────────────")

	for _, item := range sorted {
		status := "☐"
		if item.Completed {
			status = "☑"
		}
		fmt.Printf("%3d  %-4s %-20s  %s\n",
			item.ID, status, item.Title, fmtDisplayTime(item.CreatedAt))
	}
	return nil
}

func (s *TodoStorage) Done(id int) error {
	for i := range s.items {
		if s.items[i].ID == id {
			if s.items[i].Completed {
				fmt.Printf("任务 ID %d 已经是完成状态\n", id)
				return nil
			}
			s.items[i].Completed = true
			s.items[i].UpdatedAt = now()
			fmt.Printf("已完成：ID %d - %s\n", s.items[i].ID, s.items[i].Title)
			s.save()
			return nil
		}
	}
	return fmt.Errorf("ID %d 不存在", id)
}

func (s *TodoStorage) Delete(id int) error {
	for i, item := range s.items {
		if item.ID == id {
			s.items = append(s.items[:i], s.items[i+1:]...)
			fmt.Printf("已删除：ID %d - %s\n", item.ID, item.Title)
			s.save()
			return nil
		}
	}
	return fmt.Errorf("ID %d 不存在", id)
}

func (s *TodoStorage) Edit(id int, newTitle string) error {
	if newTitle == "" {
		return fmt.Errorf("标题不能为空")
	}
	for i := range s.items {
		if s.items[i].ID == id {
			s.items[i].Title = newTitle
			s.items[i].UpdatedAt = now()
			fmt.Printf("已更新：ID %d - %s\n", s.items[i].ID, s.items[i].Title)
			s.save()
			return nil
		}
	}
	return fmt.Errorf("ID %d 不存在", id)
}

func (s *TodoStorage) ClearDone() error {
	before := len(s.items)
	kept := make([]TodoItem, 0, len(s.items))
	for _, item := range s.items {
		if !item.Completed {
			kept = append(kept, item)
		}
	}
	removed := before - len(kept)
	if removed == 0 {
		fmt.Println("没有已完成的任务需要清除")
		return nil
	}
	s.items = kept
	fmt.Printf("已清除 %d 条已完成任务\n", removed)
	s.save()
	return nil
}

func Help() {
	fmt.Println("用法：todo [--file <路径>] <命令> [参数...]")
	fmt.Println()
	fmt.Println("命令：")
	fmt.Println("  add <标题>        添加任务")
	fmt.Println("  list              列出所有任务")
	fmt.Println("  done <ID>         标记任务为已完成")
	fmt.Println("  delete <ID>       删除任务")
	fmt.Println("  edit <ID> <标题>  修改任务标题")
	fmt.Println("  clear             清除所有已完成任务")
	fmt.Println("  help              显示此帮助")
	fmt.Println()
	fmt.Println("全局选项：")
	fmt.Println("  --file <路径>     指定数据文件路径（默认：~/.todo/todos.json）")
}
