#!/usr/bin/env bash
# Shell Todo CLI — 测试
set -u
# 不要 set -e，我们需要捕获错误

TODO_SCRIPT="$(cd "$(dirname "$0")" && pwd)/todo.sh"
PASS=0
FAIL=0
export PATH="${HOME}/bin:${PATH}"

temp_path() {
    echo "/tmp/todo_test_${1}.json"
}

cleanup() {
    rm -f /tmp/todo_test_*.json
}

assert_eq() {
    local expected="$1"
    local actual="$2"
    local msg="$3"
    if [[ "$expected" == "$actual" ]]; then
        echo "  PASS: $msg"
        PASS=$((PASS + 1))
    else
        echo "  FAIL: $msg (期望: $expected, 实际: $actual)"
        FAIL=$((FAIL + 1))
    fi
}

assert_exit() {
    local expected_code="$1"
    shift
    local msg="$1"
    shift
    local actual_code=0
    "$@" 2>/dev/null || actual_code=$?
    if [[ "$expected_code" -eq "$actual_code" ]]; then
        echo "  PASS: $msg"
        PASS=$((PASS + 1))
    else
        echo "  FAIL: $msg (期望退出码: $expected_code, 实际: $actual_code)"
        FAIL=$((FAIL + 1))
    fi
}

# ── 测试用例 ──────────────────────────────────────────────────────

test_add() {
    local f; f=$(temp_path "add")
    cleanup
    assert_exit 0 "add: 添加任务" bash "$TODO_SCRIPT" --file "$f" add "测试任务1"
    cleanup
}

test_list_empty() {
    local f; f=$(temp_path "list_empty")
    cleanup
    local out; out=$(bash "$TODO_SCRIPT" --file "$f" list)
    assert_eq "暂无任务" "$out" "list_empty: 空列表输出"
    cleanup
}

test_done() {
    local f; f=$(temp_path "done")
    cleanup
    bash "$TODO_SCRIPT" --file "$f" add "可完成" > /dev/null
    assert_exit 0 "done: 标记完成" bash "$TODO_SCRIPT" --file "$f" done 1
    cleanup
}

test_done_already() {
    local f; f=$(temp_path "done_already")
    cleanup
    bash "$TODO_SCRIPT" --file "$f" add "已完成" > /dev/null
    bash "$TODO_SCRIPT" --file "$f" done 1 > /dev/null
    local out; out=$(bash "$TODO_SCRIPT" --file "$f" done 1)
    assert_eq "任务 ID 1 已经是完成状态" "$out" "done_already: 已完成的任务再次完成"
    cleanup
}

test_delete() {
    local f; f=$(temp_path "delete")
    cleanup
    bash "$TODO_SCRIPT" --file "$f" add "待删除" > /dev/null
    assert_exit 0 "delete: 删除任务" bash "$TODO_SCRIPT" --file "$f" delete 1
    cleanup
}

test_edit() {
    local f; f=$(temp_path "edit")
    cleanup
    bash "$TODO_SCRIPT" --file "$f" add "旧标题" > /dev/null
    assert_exit 0 "edit: 编辑标题" bash "$TODO_SCRIPT" --file "$f" edit 1 "新标题"
    cleanup
}

test_id_not_found() {
    local f; f=$(temp_path "id_not_found")
    cleanup
    assert_exit 1 "id_not_found: done 不存在 ID" bash "$TODO_SCRIPT" --file "$f" done 999
    assert_exit 1 "id_not_found: delete 不存在 ID" bash "$TODO_SCRIPT" --file "$f" delete 999
    assert_exit 1 "id_not_found: edit 不存在 ID" bash "$TODO_SCRIPT" --file "$f" edit 999 "任意"
    cleanup
}

test_clear_done() {
    local f; f=$(temp_path "clear_done")
    cleanup
    bash "$TODO_SCRIPT" --file "$f" add "任务 A" > /dev/null
    bash "$TODO_SCRIPT" --file "$f" add "任务 B" > /dev/null
    bash "$TODO_SCRIPT" --file "$f" done 2 > /dev/null
    assert_exit 0 "clear_done: 清除已完成" bash "$TODO_SCRIPT" --file "$f" clear
    cleanup
}

test_clear_idempotent() {
    local f; f=$(temp_path "clear_idempotent")
    cleanup
    bash "$TODO_SCRIPT" --file "$f" add "只有未完成" > /dev/null
    local out; out=$(bash "$TODO_SCRIPT" --file "$f" clear)
    assert_eq "没有已完成的任务需要清除" "$out" "clear_idempotent: 无已完成时幂等"
    cleanup
}

test_file_not_exists() {
    local f; f=$(temp_path "nonexistent_ok")
    cleanup
    local out; out=$(bash "$TODO_SCRIPT" --file "$f" list)
    assert_eq "暂无任务" "$out" "file_not_exists: 文件不存在时初始化"
    rm -f "$f"
}

# ── 主函数 ────────────────────────────────────────────────────────

echo "=== Shell Todo CLI 测试 ==="
test_add
test_list_empty
test_done
test_done_already
test_delete
test_edit
test_id_not_found
test_clear_done
test_clear_idempotent
test_file_not_exists

echo ""
echo "结果：${PASS} 通过${FAIL:+, ${FAIL} 失败}"
echo ""
exit $(( FAIL > 0 ? 1 : 0 ))
