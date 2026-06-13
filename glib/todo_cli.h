#ifndef TODO_CLI_H
#define TODO_CLI_H

#include <glib.h>
#include <json-glib/json-glib.h>

typedef struct {
    gint id;
    gchar *title;
    gboolean completed;
    gchar *created_at;
    gchar *updated_at;
} TodoItem;

void todo_item_free(TodoItem *item);
TodoItem *todo_item_copy(const TodoItem *item);

typedef struct {
    GPtrArray *items;  /* of TodoItem* */
    gint next_id;
    gchar *filepath;
} TodoStorage;

TodoStorage *todo_storage_new(const gchar *filepath);
void todo_storage_free(TodoStorage *storage);

/* 命令接口：返回 0 成功，非 0 错误 */
gint todo_add(TodoStorage *storage, const gchar *title);
gint todo_list(TodoStorage *storage);
gint todo_done(TodoStorage *storage, gint id);
gint todo_delete(TodoStorage *storage, gint id);
gint todo_edit(TodoStorage *storage, gint id, const gchar *new_title);
gint todo_clear_done(TodoStorage *storage);
gint todo_help(void);

/* 时间工具 */
gchar *todo_timestamp_now(void);
gchar *todo_format_display_time(const gchar *iso);

#endif /* TODO_CLI_H */
