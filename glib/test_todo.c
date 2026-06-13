#include "todo_cli.h"

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* 测试用的临时文件路径 */
static gchar *test_filepath = NULL;

static void setup(void) {
    /* 创建临时文件路径 */
    const gchar *tmpdir = g_get_tmp_dir();
    test_filepath = g_strdup_printf("%s/todo_test_XXXXXX", tmpdir);

    /* 用 mkstemp 创建唯一临时文件，然后立即关闭 */
    gint fd = g_mkstemp(test_filepath);
    if (fd != -1) close(fd);
}

static void teardown(void) {
    if (test_filepath) {
        remove(test_filepath);
        g_free(test_filepath);
        test_filepath = NULL;
    }
}

/* 辅助函数：创建测试存储 */
static TodoStorage *make_storage(void) {
    return todo_storage_new(test_filepath);
}

/* 1. 添加任务 */
static void test_add(void) {
    setup();
    TodoStorage *s = make_storage();
    g_assert_cmpint(todo_add(s, "测试任务1"), ==, 0);
    g_assert_cmpint(s->items->len, ==, 1);
    g_assert_cmpint(s->next_id, ==, 2);
    TodoItem *item = (TodoItem *)g_ptr_array_index(s->items, 0);
    g_assert_cmpstr(item->title, ==, "测试任务1");
    g_assert_false(item->completed);
    g_assert_cmpint(item->id, ==, 1);
    g_assert_nonnull(item->created_at);
    g_assert_nonnull(item->updated_at);
    todo_storage_free(s);
    teardown();
}

/* 2. 列表为空 */
static void test_list_empty(void) {
    setup();
    TodoStorage *s = make_storage();
    /* 空列表应该成功输出，不会崩溃 */
    g_assert_cmpint(todo_list(s), ==, 0);
    todo_storage_free(s);
    teardown();
}

/* 3. 标记完成 */
static void test_done(void) {
    setup();
    TodoStorage *s = make_storage();
    todo_add(s, "待完成");
    g_assert_cmpint(todo_done(s, 1), ==, 0);
    TodoItem *item = (TodoItem *)g_ptr_array_index(s->items, 0);
    g_assert_true(item->completed);
    todo_storage_free(s);
    teardown();
}

/* 4. 已经完成的任务再次完成（幂等） */
static void test_done_idempotent(void) {
    setup();
    TodoStorage *s = make_storage();
    todo_add(s, "已完成的任务");
    todo_done(s, 1);
    g_assert_cmpint(todo_done(s, 1), ==, 0);
    /* 状态仍然为完成 */
    TodoItem *item = (TodoItem *)g_ptr_array_index(s->items, 0);
    g_assert_true(item->completed);
    todo_storage_free(s);
    teardown();
}

/* 5. 删除任务 */
static void test_delete(void) {
    setup();
    TodoStorage *s = make_storage();
    todo_add(s, "待删除");
    g_assert_cmpint(s->items->len, ==, 1);
    g_assert_cmpint(todo_delete(s, 1), ==, 0);
    g_assert_cmpint(s->items->len, ==, 0);
    todo_storage_free(s);
    teardown();
}

/* 6. 编辑任务 */
static void test_edit(void) {
    setup();
    TodoStorage *s = make_storage();
    todo_add(s, "旧标题");
    g_assert_cmpint(todo_edit(s, 1, "新标题"), ==, 0);
    TodoItem *item = (TodoItem *)g_ptr_array_index(s->items, 0);
    g_assert_cmpstr(item->title, ==, "新标题");
    todo_storage_free(s);
    teardown();
}

/* 7. ID 不存在 */
static void test_id_not_found(void) {
    setup();
    TodoStorage *s = make_storage();
    todo_add(s, "某个任务");
    g_assert_cmpint(todo_done(s, 999), ==, 1);
    g_assert_cmpint(todo_delete(s, 999), ==, 1);
    g_assert_cmpint(todo_edit(s, 999, "新标题"), ==, 1);
    todo_storage_free(s);
    teardown();
}

/* 8. clear 清除已完成任务 */
static void test_clear_done(void) {
    setup();
    TodoStorage *s = make_storage();
    todo_add(s, "任务1");
    todo_add(s, "任务2");
    todo_add(s, "任务3");
    todo_done(s, 1);
    todo_done(s, 3);
    g_assert_cmpint(s->items->len, ==, 3);
    g_assert_cmpint(todo_clear_done(s), ==, 0);
    g_assert_cmpint(s->items->len, ==, 1);
    TodoItem *item = (TodoItem *)g_ptr_array_index(s->items, 0);
    g_assert_cmpint(item->id, ==, 2);
    todo_storage_free(s);
    teardown();
}

/* 9. clear 没有已完成任务（幂等） */
static void test_clear_done_idempotent(void) {
    setup();
    TodoStorage *s = make_storage();
    todo_add(s, "未完成的任务");
    g_assert_cmpint(todo_clear_done(s), ==, 0);
    g_assert_cmpint(s->items->len, ==, 1);
    todo_storage_free(s);
    teardown();
}

/* 10. 存储文件不存在时的初始化 */
static void test_storage_init_empty(void) {
    setup();
    /* 确保文件不存在 */
    remove(test_filepath);
    TodoStorage *s = make_storage();
    g_assert_cmpint(s->items->len, ==, 0);
    g_assert_cmpint(s->next_id, ==, 1);
    /* 添加后文件应该被创建 */
    todo_add(s, "新任务");
    g_assert_true(g_file_test(test_filepath, G_FILE_TEST_EXISTS));
    todo_storage_free(s);
    teardown();
}

int main(int argc, char *argv[]) {
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/todo/add", test_add);
    g_test_add_func("/todo/list_empty", test_list_empty);
    g_test_add_func("/todo/done", test_done);
    g_test_add_func("/todo/done_idempotent", test_done_idempotent);
    g_test_add_func("/todo/delete", test_delete);
    g_test_add_func("/todo/edit", test_edit);
    g_test_add_func("/todo/id_not_found", test_id_not_found);
    g_test_add_func("/todo/clear_done", test_clear_done);
    g_test_add_func("/todo/clear_done_idempotent", test_clear_done_idempotent);
    g_test_add_func("/todo/storage_init_empty", test_storage_init_empty);

    return g_test_run();
}
