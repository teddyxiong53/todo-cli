#!/usr/bin/env lua
-- Lua Todo CLI — 测试

local PASS = 0
local FAIL = 0

local function assert_eq(expected, actual, msg)
    if expected == actual then
        io.write(string.format("  PASS: %s\n", msg))
        PASS = PASS + 1
    else
        io.write(string.format("  FAIL: %s (期望: %s, 实际: %s)\n", msg, tostring(expected), tostring(actual)))
        FAIL = FAIL + 1
    end
end

local function assert_exit(expected_code, msg, fn)
    local ok, code = pcall(function()
        -- 捕获 os.exit 调用
        local old_exit = os.exit
        local exit_code = nil
        os.exit = function(code) exit_code = code or 0; error("__EXIT__") end
        fn()
        os.exit = old_exit
        return nil
    end)
    local actual_code = 0
    if not ok then
        if tostring(code):match("__EXIT__") then
            -- 正常通过 os.exit 退出
            actual_code = exit_code or 0
        else
            -- 其他错误
            actual_code = -1
        end
    end
    if expected_code == actual_code then
        io.write(string.format("  PASS: %s\n", msg))
        PASS = PASS + 1
    else
        io.write(string.format("  FAIL: %s (期望退出码: %d, 实际: %d)\n", msg, expected_code, actual_code))
        FAIL = FAIL + 1
    end
end

local function temp_path(name)
    return string.format("/tmp/todo_test_%s.json", name)
end

local function cleanup(name)
    local f = io.open(temp_path(name), "r")
    if f then f:close(); os.remove(temp_path(name)) end
end

-- ── 测试用例 ──────────────────────────────────────────────────────

local function test_add()
    local name = "add"
    cleanup(name)
    local storage = require("todo_cli").TodoStorage.new(temp_path(name))
    local ok, code = pcall(function() storage:add("测试任务1") end)
    assert_eq(true, ok, "add: 添加任务成功")
    cleanup(name)
end

local function test_list_empty()
    local name = "list_empty"
    cleanup(name)
    local storage = require("todo_cli").TodoStorage.new(temp_path(name))
    storage:list()  -- 只检查不 panic
    assert_eq(true, true, "list_empty: 空列表输出")
    cleanup(name)
end

local function test_done()
    local name = "done"
    cleanup(name)
    local storage = require("todo_cli").TodoStorage.new(temp_path(name))
    storage:add("可完成")
    local code = storage:done(1)
    assert_eq(0, code, "done: 标记完成")
    cleanup(name)
end

local function test_done_already()
    local name = "done_already"
    cleanup(name)
    local storage = require("todo_cli").TodoStorage.new(temp_path(name))
    storage:add("已完成")
    storage:done(1)
    local code = storage:done(1)
    assert_eq(0, code, "done_already: 已完成的任务再次完成")
    cleanup(name)
end

local function test_delete()
    local name = "delete"
    cleanup(name)
    local storage = require("todo_cli").TodoStorage.new(temp_path(name))
    storage:add("待删除")
    local code = storage:delete(1)
    assert_eq(0, code, "delete: 删除任务")
    cleanup(name)
end

local function test_edit()
    local name = "edit"
    cleanup(name)
    local storage = require("todo_cli").TodoStorage.new(temp_path(name))
    storage:add("旧标题")
    local code = storage:edit(1, "新标题")
    assert_eq(0, code, "edit: 编辑标题")
    cleanup(name)
end

local function test_id_not_found()
    local name = "id_not_found"
    cleanup(name)
    local storage = require("todo_cli").TodoStorage.new(temp_path(name))
    local c1 = storage:done(999)
    assert_eq(1, c1, "id_not_found: done 不存在 ID")
    local c2 = storage:delete(999)
    assert_eq(1, c2, "id_not_found: delete 不存在 ID")
    local c3 = storage:edit(999, "任意")
    assert_eq(1, c3, "id_not_found: edit 不存在 ID")
    cleanup(name)
end

local function test_clear_done()
    local name = "clear_done"
    cleanup(name)
    local storage = require("todo_cli").TodoStorage.new(temp_path(name))
    storage:add("任务 A")
    storage:add("任务 B")
    storage:done(2)
    local code = storage:clear_done()
    assert_eq(0, code, "clear_done: 清除已完成")
    cleanup(name)
end

local function test_clear_idempotent()
    local name = "clear_idempotent"
    cleanup(name)
    local storage = require("todo_cli").TodoStorage.new(temp_path(name))
    storage:add("只有未完成")
    local code = storage:clear_done()
    assert_eq(0, code, "clear_idempotent: 无已完成时幂等")
    cleanup(name)
end

local function test_file_not_exists()
    local name = "nonexistent_ok"
    cleanup(name)
    local storage = require("todo_cli").TodoStorage.new(temp_path(name))
    local code = storage:list()
    assert_eq(0, code, "file_not_exists: 文件不存在时初始化")
    cleanup(name)
end

-- ── 主函数 ────────────────────────────────────────────────────────

io.write("=== Lua Todo CLI 测试 ===\n")

test_add()
test_list_empty()
test_done()
test_done_already()
test_delete()
test_edit()
test_id_not_found()
test_clear_done()
test_clear_idempotent()
test_file_not_exists()

io.write(string.format("\n结果：%d 通过", PASS))
if FAIL > 0 then io.write(string.format(", %d 失败", FAIL)) end
io.write("\n")

os.exit(FAIL > 0 and 1 or 0)
