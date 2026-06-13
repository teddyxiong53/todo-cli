#include "todo_cli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* ── 工具函数 ──────────────────────────────────────────────────── */

gchar *todo_timestamp_now(void) {
    GDateTime *dt = g_date_time_new_now_utc();
    gchar *result = g_date_time_format(dt, "%Y-%m-%dT%H:%M:%SZ");
    g_date_time_unref(dt);
    return result;
}

gchar *todo_format_display_time(const gchar *iso) {
    /* "2025-01-01T10:00:00Z" → "2025-01-01 10:00:00 +0000" */
    if (iso == NULL || strlen(iso) < 20)
        return g_strdup(iso ? iso : "");

    gchar *s = g_strdup(iso);
    s[10] = ' ';          /* T → 空格 */
    gsize len = strlen(s);
    if (len > 0 && s[len - 1] == 'Z') {
        /* 去掉 Z，替换成 " +0000" */
        gchar *result = g_strdup_printf("%s +0000", s);
        s[len - 1] = '\0';
        /* 截断，重新拼 */
        gchar tmp[256];
        g_snprintf(tmp, sizeof(tmp), "%.*s +0000", (int)(len - 1), s);
        g_free(s);
        return g_strdup(tmp);
    }
    return s;
}

static void ensure_dir(const gchar *dir) {
    if (dir == NULL || *dir == '\0') return;
    g_mkdir_with_parents(dir, 0755);
}

/* ── TodoItem ──────────────────────────────────────────────────── */

void todo_item_free(TodoItem *item) {
    if (item == NULL) return;
    g_free(item->title);
    g_free(item->created_at);
    g_free(item->updated_at);
    g_free(item);
}

TodoItem *todo_item_copy(const TodoItem *item) {
    if (item == NULL) return NULL;
    TodoItem *copy = g_new0(TodoItem, 1);
    copy->id = item->id;
    copy->title = g_strdup(item->title);
    copy->completed = item->completed;
    copy->created_at = g_strdup(item->created_at);
    copy->updated_at = g_strdup(item->updated_at);
    return copy;
}

/* ── 构造 / 加载 / 保存 ────────────────────────────────────────── */

static void storage_load(TodoStorage *storage);
static void storage_save(TodoStorage *storage);

TodoStorage *todo_storage_new(const gchar *filepath) {
    TodoStorage *storage = g_new0(TodoStorage, 1);
    storage->filepath = g_strdup(filepath);
    storage->items = g_ptr_array_new_full(8, (GDestroyNotify)todo_item_free);
    storage->next_id = 1;
    storage_load(storage);
    return storage;
}

void todo_storage_free(TodoStorage *storage) {
    if (storage == NULL) return;
    g_free(storage->filepath);
    g_ptr_array_unref(storage->items);
    g_free(storage);
}

static void storage_load(TodoStorage *storage) {
    g_ptr_array_set_size(storage->items, 0);
    storage->next_id = 1;

    GError *error = NULL;
    JsonParser *parser = json_parser_new();

    if (!json_parser_load_from_file(parser, storage->filepath, &error)) {
        /* 文件不存在 → 空列表 */
        g_object_unref(parser);
        if (error) {
            if (error->code != G_FILE_ERROR_NOENT) {
                g_printerr("错误：读取文件失败 - %s\n", error->message);
            }
            g_error_free(error);
        }
        return;
    }

    JsonNode *root = json_parser_get_root(parser);
    if (root == NULL || JSON_NODE_HOLDS_NULL(root)) {
        g_object_unref(parser);
        return;
    }

    JsonObject *obj = json_node_get_object(root);
    if (obj == NULL) {
        g_object_unref(parser);
        return;
    }

    /* 读取 next_id */
    if (json_object_has_member(obj, "next_id")) {
        storage->next_id = json_object_get_int_member(obj, "next_id");
    }

    /* 读取 todos */
    if (json_object_has_member(obj, "todos")) {
        JsonArray *arr = json_object_get_array_member(obj, "todos");
        guint len = json_array_get_length(arr);
        for (guint i = 0; i < len; i++) {
            JsonNode *node = json_array_get_element(arr, i);
            JsonObject *item_obj = json_node_get_object(node);

            TodoItem *item = g_new0(TodoItem, 1);
            item->id = json_object_get_int_member(item_obj, "id");
            item->title = g_strdup(json_object_get_string_member(item_obj, "title"));
            item->completed = json_object_get_boolean_member(item_obj, "completed");
            item->created_at = g_strdup(json_object_get_string_member(item_obj, "created_at"));
            item->updated_at = g_strdup(json_object_get_string_member(item_obj, "updated_at"));

            g_ptr_array_add(storage->items, item);
        }
    }

    g_object_unref(parser);
}

static void storage_save(TodoStorage *storage) {
    /* 确保目录存在 */
    gchar *dir = g_path_get_dirname(storage->filepath);
    ensure_dir(dir);
    g_free(dir);

    GError *error = NULL;

    /* 使用 JsonBuilder 构建 JSON */
    JsonBuilder *builder = json_builder_new();
    json_builder_begin_object(builder);

    /* todos 数组 */
    json_builder_set_member_name(builder, "todos");
    json_builder_begin_array(builder);
    for (guint i = 0; i < storage->items->len; i++) {
        TodoItem *item = (TodoItem *)g_ptr_array_index(storage->items, i);
        json_builder_begin_object(builder);
        json_builder_set_member_name(builder, "id");
        json_builder_add_int_value(builder, item->id);
        json_builder_set_member_name(builder, "title");
        json_builder_add_string_value(builder, item->title);
        json_builder_set_member_name(builder, "completed");
        json_builder_add_boolean_value(builder, item->completed);
        json_builder_set_member_name(builder, "created_at");
        json_builder_add_string_value(builder, item->created_at);
        json_builder_set_member_name(builder, "updated_at");
        json_builder_add_string_value(builder, item->updated_at);
        json_builder_end_object(builder);
    }
    json_builder_end_array(builder);

    /* next_id */
    json_builder_set_member_name(builder, "next_id");
    json_builder_add_int_value(builder, storage->next_id);

    json_builder_end_object(builder);

    JsonNode *root = json_builder_get_root(builder);
    g_object_unref(builder);

    /* 写入文件 */
    JsonGenerator *gen = json_generator_new();
    json_generator_set_pretty(gen, TRUE);
    json_generator_set_root(gen, root);

    if (!json_generator_to_file(gen, storage->filepath, &error)) {
        g_printerr("错误：无法写入 %s - %s\n", storage->filepath,
                   error ? error->message : "未知错误");
        if (error) g_error_free(error);
        json_node_free(root);
        g_object_unref(gen);
        exit(1);
    }

    json_node_free(root);
    g_object_unref(gen);
}

/* ── 工具比较函数 ──────────────────────────────────────────────── */

static gint todo_sort_by_id(gconstpointer a, gconstpointer b) {
    const TodoItem *item_a = *(const TodoItem **)a;
    const TodoItem *item_b = *(const TodoItem **)b;
    return item_a->id - item_b->id;
}

/* ── 命令实现 ──────────────────────────────────────────────────── */

gint todo_add(TodoStorage *storage, const gchar *title) {
    if (title == NULL || *title == '\0') {
        g_printerr("错误：标题不能为空\n");
        return 1;
    }

    TodoItem *item = g_new0(TodoItem, 1);
    item->id = storage->next_id++;
    item->title = g_strdup(title);
    item->completed = FALSE;
    item->created_at = todo_timestamp_now();
    item->updated_at = g_strdup(item->created_at);

    g_ptr_array_add(storage->items, item);
    storage_save(storage);

    g_print("已添加：ID %d - %s\n", item->id, item->title);
    return 0;
}

gint todo_list(TodoStorage *storage) {
    if (storage->items->len == 0) {
        g_print("暂无任务\n");
        return 0;
    }

    /* 按 id 升序排列 */
    g_ptr_array_sort(storage->items, (GCompareFunc)todo_sort_by_id);

    /* 表头 */
    g_print("ID  状态  标题                创建时间\n");
    g_print("─── ──── ──────────────────── ─────────────────────────\n");

    for (guint i = 0; i < storage->items->len; i++) {
        TodoItem *item = (TodoItem *)g_ptr_array_index(storage->items, i);
        const gchar *status = item->completed ? "☑" : "☐";
        gchar *disp_time = todo_format_display_time(item->created_at);

        g_print("%3d  %s    %-20s %s\n",
                item->id, status, item->title, disp_time);

        g_free(disp_time);
    }

    return 0;
}

gint todo_done(TodoStorage *storage, gint id) {
    TodoItem *found = NULL;
    guint found_idx = 0;

    for (guint i = 0; i < storage->items->len; i++) {
        TodoItem *item = (TodoItem *)g_ptr_array_index(storage->items, i);
        if (item->id == id) {
            found = item;
            found_idx = i;
            break;
        }
    }

    if (found == NULL) {
        g_printerr("错误：ID %d 不存在\n", id);
        return 1;
    }

    if (found->completed) {
        g_print("任务 ID %d 已经是完成状态\n", id);
        return 0;
    }

    found->completed = TRUE;
    g_free(found->updated_at);
    found->updated_at = todo_timestamp_now();
    storage_save(storage);

    g_print("已完成：ID %d - %s\n", found->id, found->title);
    return 0;
}

gint todo_delete(TodoStorage *storage, gint id) {
    gint found_idx = -1;
    gchar *title = NULL;

    for (guint i = 0; i < storage->items->len; i++) {
        TodoItem *item = (TodoItem *)g_ptr_array_index(storage->items, i);
        if (item->id == id) {
            found_idx = (gint)i;
            title = g_strdup(item->title);
            break;
        }
    }

    if (found_idx < 0) {
        g_printerr("错误：ID %d 不存在\n", id);
        return 1;
    }

    g_ptr_array_remove_index(storage->items, (guint)found_idx);
    storage_save(storage);

    g_print("已删除：ID %d - %s\n", id, title);
    g_free(title);
    return 0;
}

gint todo_edit(TodoStorage *storage, gint id, const gchar *new_title) {
    if (new_title == NULL || *new_title == '\0') {
        g_printerr("错误：标题不能为空\n");
        return 1;
    }

    TodoItem *found = NULL;

    for (guint i = 0; i < storage->items->len; i++) {
        TodoItem *item = (TodoItem *)g_ptr_array_index(storage->items, i);
        if (item->id == id) {
            found = item;
            break;
        }
    }

    if (found == NULL) {
        g_printerr("错误：ID %d 不存在\n", id);
        return 1;
    }

    g_free(found->title);
    found->title = g_strdup(new_title);
    g_free(found->updated_at);
    found->updated_at = todo_timestamp_now();
    storage_save(storage);

    g_print("已更新：ID %d - %s\n", found->id, found->title);
    return 0;
}

gint todo_clear_done(TodoStorage *storage) {
    guint before = storage->items->len;

    /* 逆序遍历删除已完成 */
    for (gint i = (gint)storage->items->len - 1; i >= 0; i--) {
        TodoItem *item = (TodoItem *)g_ptr_array_index(storage->items, (guint)i);
        if (item->completed) {
            g_ptr_array_remove_index(storage->items, (guint)i);
        }
    }

    guint removed = before - storage->items->len;

    if (removed == 0) {
        g_print("没有已完成的任务需要清除\n");
        return 0;
    }

    storage_save(storage);
    g_print("已清除 %u 条已完成任务\n", removed);
    return 0;
}

gint todo_help(void) {
    g_print("用法：todo [--file <路径>] <命令> [参数...]\n");
    g_print("\n");
    g_print("命令：\n");
    g_print("  add <标题>        添加任务\n");
    g_print("  list              列出所有任务\n");
    g_print("  done <ID>         标记任务为已完成\n");
    g_print("  delete <ID>       删除任务\n");
    g_print("  edit <ID> <标题>  修改任务标题\n");
    g_print("  clear             清除所有已完成任务\n");
    g_print("  help              显示此帮助\n");
    g_print("\n");
    g_print("全局选项：\n");
    g_print("  --file <路径>     指定数据文件路径（默认：~/.todo/todos.json）\n");
    return 0;
}
