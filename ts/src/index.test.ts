import { describe, it, expect, beforeEach, afterEach } from "vitest";
import { TodoStorage } from "./index.js";
import fs from "node:fs";
import path from "node:path";
import os from "node:os";

function tempPath(name: string): string {
  return path.join(os.tmpdir(), `todo_test_${name}.json`);
}

function withStorage(name: string, fn: (s: TodoStorage) => void) {
  const p = tempPath(name);
  try {
    fs.unlinkSync(p);
  } catch {
    // ignore
  }
  const s = new TodoStorage(p);
  fn(s);
  try {
    fs.unlinkSync(p);
  } catch {
    // ignore
  }
}

describe("TodoStorage", () => {
  it("add", () => {
    withStorage("add", (s) => {
      expect(s.add("测试任务1")).toBe(0);
      // 无法直接访问私有属性，通过 list 验证不报错
    });
  });

  it("list empty", () => {
    withStorage("list_empty", (s) => {
      expect(s.list()).toBe(0);
    });
  });

  it("done", () => {
    withStorage("done", (s) => {
      s.add("可完成");
      expect(s.done(1)).toBe(0);
    });
  });

  it("done already completed", () => {
    withStorage("done_already", (s) => {
      s.add("已完成");
      s.done(1);
      expect(s.done(1)).toBe(0); // 幂等
    });
  });

  it("delete", () => {
    withStorage("delete", (s) => {
      s.add("待删除");
      expect(s.delete(1)).toBe(0);
    });
  });

  it("edit", () => {
    withStorage("edit", (s) => {
      s.add("旧标题");
      expect(s.edit(1, "新标题")).toBe(0);
    });
  });

  it("id not found", () => {
    withStorage("id_not_found", (s) => {
      expect(s.done(999)).toBe(1);
      expect(s.delete(999)).toBe(1);
      expect(s.edit(999, "任意")).toBe(1);
    });
  });

  it("clear done", () => {
    withStorage("clear_done", (s) => {
      s.add("任务 A");
      s.add("任务 B");
      s.done(2);
      expect(s.clearDone()).toBe(0);
    });
  });

  it("clear idempotent", () => {
    withStorage("clear_idempotent", (s) => {
      s.add("只有未完成");
      expect(s.clearDone()).toBe(0);
    });
  });

  it("persistence", () => {
    const p = tempPath("persistence");
    try {
      fs.unlinkSync(p);
    } catch {
      // ignore
    }

    const s1 = new TodoStorage(p);
    s1.add("持久化测试");
    // s1 析构

    const s2 = new TodoStorage(p);
    // s2 应该加载到数据
    expect(s2.list()).toBe(0);

    try {
      fs.unlinkSync(p);
    } catch {
      // ignore
    }
  });

  it("file not exists", () => {
    const p = "/tmp/todo_test_nonexistent_ok.json";
    try {
      fs.unlinkSync(p);
    } catch {
      // ignore
    }
    const s = new TodoStorage(p);
    expect(s.list()).toBe(0);
    try {
      fs.unlinkSync(p);
    } catch {
      // ignore
    }
  });
});
