package main

import (
	"os"
	"path/filepath"
	"testing"
)

func tempPath(name string) string {
	return filepath.Join(os.TempDir(), "todo_test_"+name+".json")
}

func withStorage(name string, f func(*TodoStorage)) {
	path := tempPath(name)
	os.Remove(path)
	s := NewTodoStorage(path)
	f(s)
	os.Remove(path)
}

func TestAddTodo(t *testing.T) {
	withStorage("add", func(s *TodoStorage) {
		if err := s.Add("测试任务1"); err != nil {
			t.Fatal(err)
		}
		if len(s.items) != 1 {
			t.Fatalf("期望 1 个任务，实际 %d", len(s.items))
		}
		if s.items[0].Title != "测试任务1" {
			t.Fatalf("标题不匹配: %s", s.items[0].Title)
		}
	})
}

func TestListEmpty(t *testing.T) {
	path := tempPath("list_empty")
	os.Remove(path)
	s := NewTodoStorage(path)
	if err := s.List(); err != nil {
		t.Fatal(err)
	}
	os.Remove(path)
}

func TestDone(t *testing.T) {
	withStorage("done", func(s *TodoStorage) {
		s.Add("可完成")
		if err := s.Done(1); err != nil {
			t.Fatal(err)
		}
		if !s.items[0].Completed {
			t.Fatal("任务应为已完成状态")
		}
	})
}

func TestDoneAlreadyCompleted(t *testing.T) {
	withStorage("done_already", func(s *TodoStorage) {
		s.Add("已完成")
		s.Done(1)
		if err := s.Done(1); err != nil {
			t.Fatal("幂等操作不应返回错误")
		}
	})
}

func TestDelete(t *testing.T) {
	withStorage("delete", func(s *TodoStorage) {
		s.Add("待删除")
		if err := s.Delete(1); err != nil {
			t.Fatal(err)
		}
		if len(s.items) != 0 {
			t.Fatal("删除后应有 0 个任务")
		}
	})
}

func TestEdit(t *testing.T) {
	withStorage("edit", func(s *TodoStorage) {
		s.Add("旧标题")
		if err := s.Edit(1, "新标题"); err != nil {
			t.Fatal(err)
		}
		if s.items[0].Title != "新标题" {
			t.Fatalf("标题未更新: %s", s.items[0].Title)
		}
	})
}

func TestIdNotFound(t *testing.T) {
	withStorage("id_not_found", func(s *TodoStorage) {
		if err := s.Done(999); err == nil {
			t.Fatal("应返回错误")
		}
		if err := s.Delete(999); err == nil {
			t.Fatal("应返回错误")
		}
		if err := s.Edit(999, "任意"); err == nil {
			t.Fatal("应返回错误")
		}
	})
}

func TestClearDone(t *testing.T) {
	withStorage("clear_done", func(s *TodoStorage) {
		s.Add("任务 A")
		s.Add("任务 B")
		s.Done(2)
		if err := s.ClearDone(); err != nil {
			t.Fatal(err)
		}
		if len(s.items) != 1 {
			t.Fatalf("清除后应有 1 个任务，实际 %d", len(s.items))
		}
		if s.items[0].ID != 1 {
			t.Fatalf("剩余任务 ID 应为 1，实际 %d", s.items[0].ID)
		}
	})
}

func TestClearIdempotent(t *testing.T) {
	withStorage("clear_idempotent", func(s *TodoStorage) {
		s.Add("只有未完成")
		if err := s.ClearDone(); err != nil {
			t.Fatal(err)
		}
		if len(s.items) != 1 {
			t.Fatal("清除后应有 1 个任务")
		}
	})
}

func TestPersistence(t *testing.T) {
	path := tempPath("persistence")
	os.Remove(path)

	func() {
		s := NewTodoStorage(path)
		s.Add("持久化测试")
	}()

	func() {
		s := NewTodoStorage(path)
		if len(s.items) != 1 {
			t.Fatalf("重新加载后应有 1 个任务，实际 %d", len(s.items))
		}
		if s.items[0].Title != "持久化测试" {
			t.Fatalf("标题不匹配: %s", s.items[0].Title)
		}
	}()

	os.Remove(path)
}

func TestFileNotExists(t *testing.T) {
	path := "/tmp/todo_test_nonexistent_ok.json"
	os.Remove(path)
	s := NewTodoStorage(path)
	if err := s.List(); err != nil {
		t.Fatal(err)
	}
	os.Remove(path)
}
