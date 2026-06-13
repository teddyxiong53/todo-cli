#!/usr/bin/env lua
-- Lua Todo CLI
-- 纯 Lua 实现，无外部依赖

-- ── JSON ─────────────────────────────────────────────────────────

local json = {}

function json.encode(val)
    local t = type(val)
    if t == "nil" then
        return "null"
    elseif t == "boolean" then
        return tostring(val)
    elseif t == "number" then
        return tostring(val)
    elseif t == "string" then
        return '"' .. val:gsub('\\', '\\\\'):gsub('"', '\\"'):gsub('\n', '\\n'):gsub('\r', '\\r'):gsub('\t', '\\t') .. '"'
    elseif t == "table" then
        local keys = {}
        for k in pairs(val) do keys[#keys + 1] = k end
        table.sort(keys, function(a, b)
            if type(a) == "number" and type(b) == "number" then return a < b end
            if type(a) == "number" then return true end
            if type(b) == "number" then return false end
            return tostring(a) < tostring(b)
        end)
        local is_array = true
        for _, k in ipairs(keys) do
            if type(k) ~= "number" or k < 1 or k ~= math.floor(k) then
                is_array = false
                break
            end
        end
        if is_array and #val > 0 then
            local parts = {}
            for i = 1, #val do
                parts[i] = json.encode(val[i])
            end
            return "[" .. table.concat(parts, ", ") .. "]"
        elseif is_array and #val == 0 then
            -- 判断是空数组还是空对象：看是否有非数字键
            local has_key = false
            for k in pairs(val) do
                if type(k) ~= "number" then has_key = true; break end
            end
            if not has_key then
                -- 检查 metatable 标记
                if getmetatable(val) and getmetatable(val).__is_object then
                    return "{}"
                end
                return "[]"
            else
                return "{}"
            end
        else
            local parts = {}
            for _, k in ipairs(keys) do
                parts[#parts + 1] = json.encode(k) .. ": " .. json.encode(val[k])
            end
            return "{" .. table.concat(parts, ", ") .. "}"
        end
    end
    return "null"
end

function json.decode(str)
    local pos = 1
    local function skip_ws()
        while pos <= #str do
            local c = str:sub(pos, pos)
            if c == ' ' or c == '\t' or c == '\n' or c == '\r' then
                pos = pos + 1
            else
                break
            end
        end
    end
    local function parse_val()
        skip_ws()
        if pos > #str then return nil end
        local c = str:sub(pos, pos)
        if c == '"' then
            pos = pos + 1
            local s = {}
            while pos <= #str do
                local ch = str:sub(pos, pos)
                if ch == '"' then
                    pos = pos + 1
                    return table.concat(s)
                elseif ch == '\\' then
                    pos = pos + 1
                    if pos > #str then break end
                    local esc = str:sub(pos, pos)
                    if esc == '"' then s[#s + 1] = '"'
                    elseif esc == '\\' then s[#s + 1] = '\\'
                    elseif esc == '/' then s[#s + 1] = '/'
                    elseif esc == 'b' then s[#s + 1] = '\b'
                    elseif esc == 'f' then s[#s + 1] = '\f'
                    elseif esc == 'n' then s[#s + 1] = '\n'
                    elseif esc == 'r' then s[#s + 1] = '\r'
                    elseif esc == 't' then s[#s + 1] = '\t'
                    elseif esc == 'u' then
                        local hex = str:sub(pos + 1, pos + 4)
                        local code = tonumber(hex, 16)
                        if code then
                            s[#s + 1] = utf8 and utf8.chr(code) or string.char(code)
                            pos = pos + 4
                        end
                    else
                        s[#s + 1] = esc
                    end
                    pos = pos + 1
                else
                    s[#s + 1] = ch
                    pos = pos + 1
                end
            end
            return table.concat(s)
        elseif c == '{' then
            pos = pos + 1
            local t = {}
            setmetatable(t, { __is_object = true })
            skip_ws()
            if str:sub(pos, pos) == '}' then
                pos = pos + 1
                return t
            end
            while true do
                skip_ws()
                local k = parse_val()
                skip_ws()
                if str:sub(pos, pos) ~= ':' then break end
                pos = pos + 1
                local v = parse_val()
                t[k] = v
                skip_ws()
                local sep = str:sub(pos, pos)
                if sep == '}' then
                    pos = pos + 1
                    break
                elseif sep == ',' then
                    pos = pos + 1
                else
                    break
                end
            end
            return t
        elseif c == '[' then
            pos = pos + 1
            local t = {}
            skip_ws()
            if str:sub(pos, pos) == ']' then
                pos = pos + 1
                return t
            end
            local idx = 1
            while true do
                t[idx] = parse_val()
                idx = idx + 1
                skip_ws()
                local sep = str:sub(pos, pos)
                if sep == ']' then
                    pos = pos + 1
                    break
                elseif sep == ',' then
                    pos = pos + 1
                else
                    break
                end
            end
            return t
        elseif c == 't' then
            if str:sub(pos, pos + 3) == 'true' then
                pos = pos + 4
                return true
            end
        elseif c == 'f' then
            if str:sub(pos, pos + 4) == 'false' then
                pos = pos + 5
                return false
            end
        elseif c == 'n' then
            if str:sub(pos, pos + 3) == 'null' then
                pos = pos + 4
                return nil
            end
        elseif c == '-' or c:match('%d') then
            local start = pos
            if c == '-' then pos = pos + 1 end
            while pos <= #str and str:sub(pos, pos):match('%d') do pos = pos + 1 end
            if str:sub(pos, pos) == '.' then
                pos = pos + 1
                while pos <= #str and str:sub(pos, pos):match('%d') do pos = pos + 1 end
            end
            if str:sub(pos, pos) == 'e' or str:sub(pos, pos) == 'E' then
                pos = pos + 1
                if str:sub(pos, pos) == '+' or str:sub(pos, pos) == '-' then pos = pos + 1 end
                while pos <= #str and str:sub(pos, pos):match('%d') do pos = pos + 1 end
            end
            local num = tonumber(str:sub(start, pos - 1))
            return num
        end
        return nil
    end
    return parse_val()
end

-- ── 辅助函数 ──────────────────────────────────────────────────────

local function now_str()
    return os.date("!%Y-%m-%dT%H:%M:%SZ")
end

local function fmt_display_time(iso)
    return iso:gsub("T", " "):gsub("Z", " +0000")
end

-- ── 存储层 ────────────────────────────────────────────────────────

local TodoStorage = {}
TodoStorage.__index = TodoStorage

function TodoStorage.new(filepath)
    local self = setmetatable({}, TodoStorage)
    self.path = filepath
    self.items = {}
    self.next_id = 1
    self:_load()
    return self
end

function TodoStorage:_ensure_dir()
    local dir = self.path:match("^(.*/)")
    if dir and dir ~= "" then
        os.execute("mkdir -p " .. dir)
    end
end

function TodoStorage:_load()
    local f, err = io.open(self.path, "r")
    if not f then
        -- 文件不存在 → 空列表
        return
    end
    local content = f:read("*a")
    f:close()
    local ok, data = pcall(json.decode, content)
    if not ok then
        io.stderr:write("错误：JSON 解析失败 - " .. tostring(data) .. "\n")
        os.exit(1)
    end
    self.items = data.todos or {}
    self.next_id = data.next_id or 1
end

function TodoStorage:_save()
    self:_ensure_dir()
    local data = { todos = self.items, next_id = self.next_id }
    local content = json.encode(data) .. "\n"
    local f, err = io.open(self.path, "w")
    if not f then
        io.stderr:write("错误：无法写入 " .. self.path .. " - " .. tostring(err) .. "\n")
        os.exit(1)
    end
    f:write(content)
    f:close()
end

-- ---- 命令实现 ----

function TodoStorage:add(title)
    if title == "" then
        io.stderr:write("错误：标题不能为空\n")
        os.exit(1)
    end
    local ts = now_str()
    local item = {
        id = self.next_id,
        title = title,
        completed = false,
        created_at = ts,
        updated_at = ts
    }
    self.next_id = self.next_id + 1
    table.insert(self.items, item)
    self:_save()
    io.write(string.format("已添加：ID %d - %s\n", item.id, item.title))
    return 0
end

function TodoStorage:list()
    if #self.items == 0 then
        io.write("暂无任务\n")
        return 0
    end

    -- 按 id 排序
    table.sort(self.items, function(a, b) return a.id < b.id end)

    io.write("ID  状态  标题                创建时间\n")
    io.write("─── ──── ──────────────────── ─────────────────────────\n")

    for _, item in ipairs(self.items) do
        local status = "☐"
        if item.completed then status = "☑" end
        io.write(string.format("%3d  %-4s %-20s  %s\n",
            item.id, status, item.title, fmt_display_time(item.created_at)))
    end
    return 0
end

function TodoStorage:done(id)
    for _, item in ipairs(self.items) do
        if item.id == id then
            if item.completed then
                io.write(string.format("任务 ID %d 已经是完成状态\n", id))
                return 0
            end
            item.completed = true
            item.updated_at = now_str()
            self:_save()
            io.write(string.format("已完成：ID %d - %s\n", item.id, item.title))
            return 0
        end
    end
    io.stderr:write(string.format("错误：ID %d 不存在\n", id))
    return 1
end

function TodoStorage:delete(id)
    for i, item in ipairs(self.items) do
        if item.id == id then
            table.remove(self.items, i)
            self:_save()
            io.write(string.format("已删除：ID %d - %s\n", item.id, item.title))
            return 0
        end
    end
    io.stderr:write(string.format("错误：ID %d 不存在\n", id))
    return 1
end

function TodoStorage:edit(id, new_title)
    if new_title == "" then
        io.stderr:write("错误：标题不能为空\n")
        os.exit(1)
    end
    for _, item in ipairs(self.items) do
        if item.id == id then
            item.title = new_title
            item.updated_at = now_str()
            self:_save()
            io.write(string.format("已更新：ID %d - %s\n", item.id, item.title))
            return 0
        end
    end
    io.stderr:write(string.format("错误：ID %d 不存在\n", id))
    return 1
end

function TodoStorage:clear_done()
    local before = #self.items
    local kept = {}
    for _, item in ipairs(self.items) do
        if not item.completed then
            table.insert(kept, item)
        end
    end
    local removed = before - #kept
    if removed == 0 then
        io.write("没有已完成的任务需要清除\n")
        return 0
    end
    self.items = kept
    self:_save()
    io.write(string.format("已清除 %d 条已完成任务\n", removed))
    return 0
end

function TodoStorage.help()
    io.write([[
用法：todo [--file <路径>] <命令> [参数...]

命令：
  add <标题>        添加任务
  list              列出所有任务
  done <ID>         标记任务为已完成
  delete <ID>       删除任务
  edit <ID> <标题>  修改任务标题
  clear             清除所有已完成任务
  help              显示此帮助

全局选项：
  --file <路径>     指定数据文件路径（默认：~/.todo/todos.json）
]])
    return 0
end

-- ── CLI 入口 ──────────────────────────────────────────────────────

local function default_filepath()
    local home = os.getenv("HOME") or os.getenv("USERPROFILE") or "."
    return home .. "/.todo/todos.json"
end

local function main(args)
    local filepath = default_filepath()
    local cmd_args = {}

    local i = 1
    while i <= #args do
        if args[i] == "--file" then
            if i + 1 > #args then
                io.stderr:write("错误：--file 需要参数\n")
                os.exit(1)
            end
            filepath = args[i + 1]
            i = i + 2
        else
            break
        end
    end

    while i <= #args do
        cmd_args[#cmd_args + 1] = args[i]
        i = i + 1
    end

    if #cmd_args == 0 then
        io.stderr:write("错误：缺少命令。使用 'todo help' 查看帮助。\n")
        os.exit(1)
    end

    local command = cmd_args[1]

    if command == "help" then
        os.exit(TodoStorage.help())
        return
    end

    local storage = TodoStorage.new(filepath)
    local exit_code = 0

    if command == "add" then
        if #cmd_args < 2 then
            io.stderr:write("错误：'add' 需要标题参数\n")
            os.exit(1)
        end
        exit_code = storage:add(cmd_args[2])
    elseif command == "list" then
        exit_code = storage:list()
    elseif command == "done" then
        if #cmd_args < 2 then
            io.stderr:write("错误：'done' 需要 ID 参数\n")
            os.exit(1)
        end
        local id = tonumber(cmd_args[2])
        if not id then
            io.stderr:write("错误：ID 必须是数字\n")
            os.exit(1)
        end
        exit_code = storage:done(id)
    elseif command == "delete" then
        if #cmd_args < 2 then
            io.stderr:write("错误：'delete' 需要 ID 参数\n")
            os.exit(1)
        end
        local id = tonumber(cmd_args[2])
        if not id then
            io.stderr:write("错误：ID 必须是数字\n")
            os.exit(1)
        end
        exit_code = storage:delete(id)
    elseif command == "edit" then
        if #cmd_args < 3 then
            io.stderr:write("错误：'edit' 需要 ID 和标题参数\n")
            os.exit(1)
        end
        local id = tonumber(cmd_args[2])
        if not id then
            io.stderr:write("错误：ID 必须是数字\n")
            os.exit(1)
        end
        exit_code = storage:edit(id, cmd_args[3])
    elseif command == "clear" then
        exit_code = storage:clear_done()
    else
        io.stderr:write(string.format("错误：未知命令 '%s'。使用 'todo help' 查看帮助。\n", command))
        os.exit(1)
    end

    os.exit(exit_code)
end

-- 直接运行时
if arg and arg[0] and (arg[0]:match("todo_cli%.lua$") or arg[0]:match("todo_cli$")) then
    -- arg is the script args, but we need the raw args
    local raw = {...}
    main(raw)
end

-- 导出（方便测试）
return {
    json = json,
    TodoStorage = TodoStorage,
    main = main,
}
