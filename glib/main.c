#include "todo_cli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static gchar *default_filepath(void) {
    const gchar *home = g_getenv("HOME");
    if (!home) home = g_getenv("USERPROFILE");
    if (!home) {
        g_printerr("错误：无法确定 HOME 目录\n");
        exit(1);
    }
    return g_build_filename(home, ".todo", "todos.json", NULL);
}

int main(int argc, char *argv[]) {
    gchar *filepath = default_filepath();
    GPtrArray *cmd_args = g_ptr_array_new_full(8, g_free);

    /* 解析全局选项 */
    gint i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "--file") == 0) {
            if (i + 1 >= argc) {
                g_printerr("错误：--file 需要参数\n");
                g_free(filepath);
                g_ptr_array_free(cmd_args, TRUE);
                return 1;
            }
            g_free(filepath);
            filepath = g_strdup(argv[i + 1]);
            i += 2;
        } else {
            g_ptr_array_add(cmd_args, g_strdup(argv[i]));
            ++i;
        }
    }

    if (cmd_args->len == 0) {
        g_printerr("错误：缺少命令。使用 'todo help' 查看帮助。\n");
        g_free(filepath);
        g_ptr_array_free(cmd_args, TRUE);
        return 1;
    }

    const gchar *command = (const gchar *)g_ptr_array_index(cmd_args, 0);
    TodoStorage *storage = todo_storage_new(filepath);
    gint ret = 0;

    if (strcmp(command, "add") == 0) {
        if (cmd_args->len < 2) {
            g_printerr("错误：'add' 需要标题参数\n");
            ret = 1;
        } else {
            ret = todo_add(storage, (const gchar *)g_ptr_array_index(cmd_args, 1));
        }
    } else if (strcmp(command, "list") == 0) {
        ret = todo_list(storage);
    } else if (strcmp(command, "done") == 0) {
        if (cmd_args->len < 2) {
            g_printerr("错误：'done' 需要 ID 参数\n");
            ret = 1;
        } else {
            gint id = atoi((const gchar *)g_ptr_array_index(cmd_args, 1));
            ret = todo_done(storage, id);
        }
    } else if (strcmp(command, "delete") == 0) {
        if (cmd_args->len < 2) {
            g_printerr("错误：'delete' 需要 ID 参数\n");
            ret = 1;
        } else {
            gint id = atoi((const gchar *)g_ptr_array_index(cmd_args, 1));
            ret = todo_delete(storage, id);
        }
    } else if (strcmp(command, "edit") == 0) {
        if (cmd_args->len < 3) {
            g_printerr("错误：'edit' 需要 ID 和标题参数\n");
            ret = 1;
        } else {
            gint id = atoi((const gchar *)g_ptr_array_index(cmd_args, 1));
            ret = todo_edit(storage, id, (const gchar *)g_ptr_array_index(cmd_args, 2));
        }
    } else if (strcmp(command, "clear") == 0) {
        ret = todo_clear_done(storage);
    } else if (strcmp(command, "help") == 0) {
        ret = todo_help();
    } else {
        g_printerr("错误：未知命令 '%s'。使用 'todo help' 查看帮助。\n", command);
        ret = 1;
    }

    todo_storage_free(storage);
    g_free(filepath);
    g_ptr_array_free(cmd_args, TRUE);
    return ret;
}
