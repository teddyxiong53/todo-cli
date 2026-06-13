"""Python Todo CLI — tests."""

import os
import tempfile
from pathlib import Path

import pytest

from todo_cli.storage import TodoStorage


def temp_path(name: str) -> str:
    return os.path.join(tempfile.gettempdir(), f"todo_test_{name}.json")


def with_storage(name: str):
    """Fixture factory — returns a (path, storage) tuple, cleaned up after."""

    @pytest.fixture
    def _make():
        p = temp_path(name)
        Path(p).unlink(missing_ok=True)
        s = TodoStorage(p)
        yield s
        Path(p).unlink(missing_ok=True)

    return _make


# ── Tests ──


def test_add():
    p = temp_path("add")
    Path(p).unlink(missing_ok=True)
    s = TodoStorage(p)
    assert s.add("测试任务1") == 0
    Path(p).unlink(missing_ok=True)


def test_list_empty():
    p = temp_path("list_empty")
    Path(p).unlink(missing_ok=True)
    s = TodoStorage(p)
    assert s.list() == 0
    Path(p).unlink(missing_ok=True)


def test_done():
    p = temp_path("done")
    Path(p).unlink(missing_ok=True)
    s = TodoStorage(p)
    s.add("可完成")
    assert s.done(1) == 0
    Path(p).unlink(missing_ok=True)


def test_done_already_completed():
    p = temp_path("done_already")
    Path(p).unlink(missing_ok=True)
    s = TodoStorage(p)
    s.add("已完成")
    s.done(1)
    assert s.done(1) == 0  # 幂等
    Path(p).unlink(missing_ok=True)


def test_delete():
    p = temp_path("delete")
    Path(p).unlink(missing_ok=True)
    s = TodoStorage(p)
    s.add("待删除")
    assert s.delete(1) == 0
    Path(p).unlink(missing_ok=True)


def test_edit():
    p = temp_path("edit")
    Path(p).unlink(missing_ok=True)
    s = TodoStorage(p)
    s.add("旧标题")
    assert s.edit(1, "新标题") == 0
    Path(p).unlink(missing_ok=True)


def test_id_not_found():
    p = temp_path("id_not_found")
    Path(p).unlink(missing_ok=True)
    s = TodoStorage(p)
    assert s.done(999) == 1
    assert s.delete(999) == 1
    assert s.edit(999, "任意") == 1
    Path(p).unlink(missing_ok=True)


def test_clear_done():
    p = temp_path("clear_done")
    Path(p).unlink(missing_ok=True)
    s = TodoStorage(p)
    s.add("任务 A")
    s.add("任务 B")
    s.done(2)
    assert s.clear_done() == 0
    Path(p).unlink(missing_ok=True)


def test_clear_idempotent():
    p = temp_path("clear_idempotent")
    Path(p).unlink(missing_ok=True)
    s = TodoStorage(p)
    s.add("只有未完成")
    assert s.clear_done() == 0
    Path(p).unlink(missing_ok=True)


def test_persistence():
    p = temp_path("persistence")
    Path(p).unlink(missing_ok=True)
    s1 = TodoStorage(p)
    s1.add("持久化测试")
    del s1

    s2 = TodoStorage(p)
    assert s2.list() == 0
    Path(p).unlink(missing_ok=True)


def test_file_not_exists():
    p = "/tmp/todo_test_nonexistent_ok.json"
    Path(p).unlink(missing_ok=True)
    s = TodoStorage(p)
    assert s.list() == 0
    Path(p).unlink(missing_ok=True)
