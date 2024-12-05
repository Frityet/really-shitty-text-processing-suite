#include "commands.c"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#pragma clang assume_nonnull begin

static inline int file_exists(const char *filename)
{ return access(filename, F_OK) == 0; }

static char *nullable read_line(const char *filename, int line_number) {
    auto file = fopen(filename, "r");
    if (!file) return nullptr;
    defer { fclose(file); };

    static char buffer[1024];
    int current_line = 1;
    while (fgets(buffer, sizeof(buffer), file)) {
        if (current_line == line_number) {
            return buffer;
        }
        current_line++;
    }
    return nullptr;
}

static void test_create_file()
{
    const char *params[] = { "test_create.txt" };
    int result = create_file(1, params);
    assert(result == 0);
    assert(file_exists("test_create.txt"));
    remove("test_create.txt");
    printf("test_create_file passed.\n");
}

static void test_copy_file()
{
    auto src = $fopen("test_source.txt", "w");
    fprintf(src, "Hello, World!\n");
    fclose(src);
    defer { remove("test_source.txt"); };

    const char *params[] = { "test_source.txt", "test_copy.txt" };
    int result = copy_file(2, params);
    assert(result == 0);
    assert(file_exists("test_copy.txt"));
    defer { remove("test_copy.txt"); };

    auto copy = fopen("test_copy.txt", "r");
    defer { fclose(copy); };
    char buffer[1024];
    fgets(buffer, sizeof(buffer), copy);
    assert(strcmp(buffer, "Hello, World!\n") == 0);

    printf("test_copy_file passed.\n");
}

static void test_delete_file()
{
    auto file = $fopen("test_delete.txt", "w");
    fprintf(file, "Temporary file.\n");
    fclose(file);

    const char *params[] = { "test_delete.txt" };
    int result = delete_file(1, params);
    assert(result == 0);
    assert(!file_exists("test_delete.txt"));
    printf("test_delete_file passed.\n");
}

static void test_show_file()
{
    auto file = $fopen("test_show.txt", "w");
    fprintf(file, "Line 1\nLine 2\n");
    fclose(file);
    defer { remove("test_show.txt"); };

    const char *params[] = { "test_show.txt" };
    printf("Output of show_file:\n");
    int result = show_file(1, params);
    assert(result == 0);

    printf("test_show_file passed.\n");
}

static void test_append_line()
{
    auto file = fopen("test_append.txt", "w");
    fprintf(file, "Line 1\n");
    fclose(file);
    defer { remove("test_append.txt"); };

    const char *params[] = { "test_append.txt", "Appended Line" };
    int result = append_line(2, params);
    assert(result == 0);

    char *line = read_line("test_append.txt", 2);
    assert(line != nullptr);
    assert(strcmp(line, "Appended Line\n") == 0);

    printf("test_append_line passed.\n");
}

static void test_delete_line()
{
    auto file = $fopen("test_delete_line.txt", "w");
    fprintf(file, "Line 1\nLine 2\nLine 3\n");
    fclose(file);
    defer { remove("test_delete_line.txt"); };

    const char *params[] = { "test_delete_line.txt", "2" };
    int result = delete_line(2, params);
    assert(result == 0);

    char *line = read_line("test_delete_line.txt", 2);
    assert(line != nullptr);
    assert(strcmp(line, "Line 3\n") == 0);

    printf("test_delete_line passed.\n");
}

static void test_insert_line()
{
    auto file = $fopen("test_insert_line.txt", "w");
    fprintf(file, "Line 1\nLine 2\n");
    fclose(file);
    defer { remove("test_insert_line.txt"); };

    const char *params[] = { "test_insert_line.txt", "2", "Inserted Line" };
    int result = insert_line(3, params);
    assert(result == 0);

    char *line = read_line("test_insert_line.txt", 2);
    assert(line != nullptr);
    assert(strcmp(line, "Inserted Line") == 0 || strcmp(line, "Inserted Line\n") == 0);

    printf("test_insert_line passed.\n");
}

static void test_show_line()
{
    auto file = fopen("test_show_line.txt", "w");
    fprintf(file, "Line 1\nLine 2\n");
    fclose(file);
    defer { remove("test_show_line.txt"); };

    const char *params[] = { "test_show_line.txt", "2" };
    printf("Output of show_line:\n");
    int result = show_line(2, params);
    assert(result == 0);

    printf("test_show_line passed.\n");
}

static void test_show_change_log()
{
    const char *params1[] = { "test_changelog.txt" };
    create_file(1, params1);

    const char *params2[] = { "test_changelog.txt", "Appended Line" };
    append_line(2, params2);

    const char *params3[] = { "test_changelog.txt", "1" };
    delete_line(2, params3);
    defer { remove("test_changelog.txt"); };

    printf("Output of show_change_log:\n");
    int result = show_change_log(0, nullptr);
    assert(result == 0);

    printf("test_show_change_log passed.\n");
}

static void test_show_number_of_lines()
{
    auto file = fopen("test_line_count.txt", "w");
    fprintf(file, "Line 1\nLine 2\nLine 3\n");
    fclose(file);
    defer { remove("test_line_count.txt"); };

    const char *params[] = { "test_line_count.txt" };
    printf("Output of show_number_of_lines:\n");
    int result = show_number_of_lines(1, params);
    assert(result == 0);

    printf("test_show_number_of_lines passed.\n");
}

int main() {
    test_create_file();
    test_copy_file();
    test_delete_file();
    test_show_file();
    test_append_line();
    test_delete_line();
    test_insert_line();
    test_show_line();
    test_show_change_log();
    test_show_number_of_lines();

    printf("All tests passed.\n");
    return 0;
}

#pragma clang assume_nonnull end
