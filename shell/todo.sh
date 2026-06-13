#!/usr/bin/env bash
# Shell Todo CLI — 一个 jq 驱动的 TODO 任务管理器
set -euo pipefail

TODO_FILE="${HOME}/.todo/todos.json"

# ── 辅助函数 ──────────────────────────────────────────────────────

now() {
    date -u +%Y-%m-%dT%H:%M:%SZ
}

fmt_time() {
    # "2025-01-01T10:00:00Z" → "2025-01-01 10:00:00 +0000"
    echo "${1/T/ }" | sed 's/Z/ +0000/'
}

ensure_file() {
    if [[ ! -f "$TODO_FILE" ]]; then
        mkdir -p "$(dirname "$TODO_FILE")"
        echo '{"todos":[],"next_id":1}' > "$TODO_FILE"
    fi
}

id_exists() {
    local id="$1"
    ensure_file
    jq -e --argjson id "$id" '.todos | any(.id == $id)' "$TODO_FILE" > /dev/null 2>&1
}

# ── 命令实现 ──────────────────────────────────────────────────────

cmd_help() {
    cat <<'EOF'
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
EOF
    return 0
}

cmd_add() {
    local title="$1"
    if [[ -z "$title" ]]; then
        echo "错误：标题不能为空" >&2
        exit 1
    fi
    ensure_file
    local n; n=$(now)
    jq \
        --arg title "$title" \
        --arg now "$n" \
        '.next_id as $id |
         .todos += [{"id": $id, "title": $title, "completed": false, "created_at": $now, "updated_at": $now}] |
         .next_id += 1' \
        "$TODO_FILE" > "${TODO_FILE}.tmp" && mv "${TODO_FILE}.tmp" "$TODO_FILE"

    local id; id=$(jq '.next_id - 1' "$TODO_FILE")
    echo "已添加：ID ${id} - ${title}"
    return 0
}

cmd_list() {
    ensure_file
    local count; count=$(jq '.todos | length' "$TODO_FILE")
    if [[ "$count" -eq 0 ]]; then
        echo "暂无任务"
        return 0
    fi

    echo "ID  状态  标题                创建时间"
    echo "─── ──── ──────────────────── ─────────────────────────"

    jq -r '.todos | sort_by(.id) | .[] | .id, .completed, .title, .created_at' "$TODO_FILE" |
    while read -r id; do
        read -r completed
        read -r title
        read -r created_at

        local status="☐"
        [[ "$completed" == "true" ]] && status="☑"
        local dt; dt=$(fmt_time "$created_at")
        printf "%3d  %-4s %-20s  %s\n" "$id" "$status" "$title" "$dt"
    done
    return 0
}

cmd_done() {
    local id="$1"
    ensure_file
    if ! id_exists "$id"; then
        echo "错误：ID ${id} 不存在" >&2
        return 1
    fi

    # 检查是否已完成
    local done_already
    done_already=$(jq -r --argjson id "$id" '.todos[] | select(.id == $id) | .completed' "$TODO_FILE")
    if [[ "$done_already" == "true" ]]; then
        echo "任务 ID ${id} 已经是完成状态"
        return 0
    fi

    local n; n=$(now)
    local title
    title=$(jq -r --argjson id "$id" '.todos[] | select(.id == $id) | .title' "$TODO_FILE")

    jq \
        --argjson id "$id" \
        --arg now "$n" \
        '(.todos[] | select(.id == $id) | .completed) = true |
         (.todos[] | select(.id == $id) | .updated_at) = $now' \
        "$TODO_FILE" > "${TODO_FILE}.tmp" && mv "${TODO_FILE}.tmp" "$TODO_FILE"

    echo "已完成：ID ${id} - ${title}"
    return 0
}

cmd_delete() {
    local id="$1"
    ensure_file
    if ! id_exists "$id"; then
        echo "错误：ID ${id} 不存在" >&2
        return 1
    fi

    local title
    title=$(jq -r --argjson id "$id" '.todos[] | select(.id == $id) | .title' "$TODO_FILE")

    jq \
        --argjson id "$id" \
        'del(.todos[] | select(.id == $id))' \
        "$TODO_FILE" > "${TODO_FILE}.tmp" && mv "${TODO_FILE}.tmp" "$TODO_FILE"

    echo "已删除：ID ${id} - ${title}"
    return 0
}

cmd_edit() {
    local id="$1"
    local new_title="$2"

    if [[ -z "$new_title" ]]; then
        echo "错误：标题不能为空" >&2
        exit 1
    fi
    ensure_file
    if ! id_exists "$id"; then
        echo "错误：ID ${id} 不存在" >&2
        return 1
    fi

    local n; n=$(now)
    jq \
        --argjson id "$id" \
        --arg title "$new_title" \
        --arg now "$n" \
        '(.todos[] | select(.id == $id) | .title) = $title |
         (.todos[] | select(.id == $id) | .updated_at) = $now' \
        "$TODO_FILE" > "${TODO_FILE}.tmp" && mv "${TODO_FILE}.tmp" "$TODO_FILE"

    echo "已更新：ID ${id} - ${new_title}"
    return 0
}

cmd_clear() {
    ensure_file
    local before; before=$(jq '.todos | length' "$TODO_FILE")
    local after

    jq 'del(.todos[] | select(.completed == true))' \
        "$TODO_FILE" > "${TODO_FILE}.tmp" && mv "${TODO_FILE}.tmp" "$TODO_FILE"

    after=$(jq '.todos | length' "$TODO_FILE")
    local removed=$(( before - after ))

    if [[ "$removed" -eq 0 ]]; then
        echo "没有已完成的任务需要清除"
    else
        echo "已清除 ${removed} 条已完成任务"
    fi
    return 0
}

# ── 主入口 ────────────────────────────────────────────────────────

main() {
    local args=("$@")

    # 解析 --file 全局选项
    local i=0
    while [[ $i -lt ${#args[@]} ]]; do
        if [[ "${args[$i]}" == "--file" ]]; then
            if [[ $((i + 1)) -ge ${#args[@]} ]]; then
                echo "错误：--file 需要参数" >&2
                exit 1
            fi
            TODO_FILE="${args[$((i + 1))]}"
            i=$((i + 2))
        else
            break
        fi
    done

    # 剩余命令参数
    local cmd_args=("${args[@]:$i}")

    if [[ ${#cmd_args[@]} -eq 0 ]]; then
        echo "错误：缺少命令。使用 'todo help' 查看帮助。" >&2
        exit 1
    fi

    local command="${cmd_args[0]}"

    case "$command" in
        help)
            cmd_help
            ;;
        add)
            if [[ ${#cmd_args[@]} -lt 2 ]]; then
                echo "错误：'add' 需要标题参数" >&2
                exit 1
            fi
            cmd_add "${cmd_args[1]}"
            ;;
        list)
            cmd_list
            ;;
        done)
            if [[ ${#cmd_args[@]} -lt 2 ]]; then
                echo "错误：'done' 需要 ID 参数" >&2
                exit 1
            fi
            if ! [[ "${cmd_args[1]}" =~ ^[0-9]+$ ]]; then
                echo "错误：ID 必须是数字" >&2
                exit 1
            fi
            cmd_done "${cmd_args[1]}"
            ;;
        delete)
            if [[ ${#cmd_args[@]} -lt 2 ]]; then
                echo "错误：'delete' 需要 ID 参数" >&2
                exit 1
            fi
            if ! [[ "${cmd_args[1]}" =~ ^[0-9]+$ ]]; then
                echo "错误：ID 必须是数字" >&2
                exit 1
            fi
            cmd_delete "${cmd_args[1]}"
            ;;
        edit)
            if [[ ${#cmd_args[@]} -lt 3 ]]; then
                echo "错误：'edit' 需要 ID 和标题参数" >&2
                exit 1
            fi
            if ! [[ "${cmd_args[1]}" =~ ^[0-9]+$ ]]; then
                echo "错误：ID 必须是数字" >&2
                exit 1
            fi
            cmd_edit "${cmd_args[1]}" "${cmd_args[*]:2}"
            ;;
        clear)
            cmd_clear
            ;;
        *)
            echo "错误：未知命令 '${command}'。使用 'todo help' 查看帮助。" >&2
            exit 1
            ;;
    esac
}

main "$@"
