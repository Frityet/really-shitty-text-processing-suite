#include "commands.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma clang assume_nonnull begin

static struct Command commands[MAX_COMMANDS];
static size_t command_count = 0;

static struct ChangelogEntry changelog[MAX_CHANGELOG_ENTRIES];
static size_t changelog_count = 0;

static void log_change(const char *operation, const char *filename, size_t line_number, size_t total_lines)
{
    if (changelog_count < MAX_CHANGELOG_ENTRIES) {
        strncpy(changelog[changelog_count].operation, operation, sizeof(changelog[changelog_count].operation) - 1);
        strncpy(changelog[changelog_count].filename, filename, sizeof(changelog[changelog_count].filename) - 1);
        changelog[changelog_count].line_number = line_number;
        changelog[changelog_count].total_lines = total_lines;
        changelog_count++;
    } else {
        fprintf(stderr, "Change log is full. Cannot log more operations.\n");
    }
}

const struct ChangelogEntry *nullable get_changelog_entry(size_t idx)
{
    if (idx < changelog_count) {
        return &changelog[idx];
    } else {
        return nullptr;
    }
}

size_t get_changelog_count(void)
{ return changelog_count; }

static int create_file(size_t param_len, const char *nonnull params[static param_len])
{
    const char *filename = params[0];
    auto file = $fopen(filename, "wb");
    defer { fclose(file); };

    printf("File '%s' created successfully.\n", filename);
    return 0;
}

static int copy_file(size_t param_len, const char *nonnull params[static param_len])
{
    const char *source = params[0], *destination = params[1];
    auto src = $fopen(source, "r");
    defer { fclose(src); };

    auto dest = $fopen(destination, "w");
    defer { fclose(dest); };
    
    char buffer[1024] = {0};
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytes, dest);
    }
    
    printf("File copied from '%s' to '%s'.\n", source, destination);
    return 0;
}

static int delete_file(size_t param_len, const char *nonnull params[static param_len])
{
    const char *filename = params[0];
    if (remove(filename) == 0) {
        printf("File '%s' deleted successfully.\n", filename);
        return 0;
    } else {
        perror("Error deleting file");
        return 1;
    }
}

static int show_file(size_t param_len, const char *nonnull params[static param_len])
{
    const char *filename = params[0];
    auto file = $fopen(filename, "r");
    defer { fclose(file); };

    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        fputs(line, stdout);
    }
    
    return 0;
}


static int append_line(size_t param_len, const char *nonnull params[static param_len])
{
    const char *filename = params[0], *line_content = params[1];

    auto file = $fopen(filename, "a");

    if (fprintf(file, "%s\n", line_content) < 0) {
        perror("Error writing to file");
        return 1;
    }

    fclose(file); //have to do it early to save changes

    // Get the number of lines after appending
    size_t total_lines = 0;
    auto read_file = fopen(filename, "r");
    if (read_file != nullptr) {
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), read_file)) {
            total_lines++;
        }
        fclose(read_file);
    }

    printf("Appended line to '%s' successfully.\n", filename);
    log_change("Append Line", filename, 0, total_lines);
    return 0;
}

static int delete_line(size_t param_len, const char *nonnull params[static param_len])
{
    const char *filename = params[0];
    int line_number = atoi(params[1]);

    auto file = $fopen(filename, "r");
    defer { fclose(file); };

    // Read all lines into memory except the one to delete
    char *nonnull *nullable lines = NULL;
    size_t lines_allocated = 0, lines_count = 0;
    char buffer[1024];
    size_t current_line = 1;
    int line_found = 0;

    while (fgets(buffer, sizeof(buffer), file)) {
        if (current_line != (size_t)line_number) {
            // Store the line
            if (lines_count >= lines_allocated) {
                lines_allocated = lines_allocated ? lines_allocated * 2 : 10;
                lines = $realloc(lines, lines_allocated * sizeof(char *));
            }
            lines[lines_count] = $strdup(buffer);
            lines_count++;
        } else {
            line_found = 1;
        }
        current_line++;
    }
    defer {
        for (size_t i = 0; i < lines_count; i++) {
            free(lines[i]);
        }
        free(lines);
    };

    if (!line_found) {
        fprintf(stderr, "Invalid line number.\n");
        return 1;
    }

    // Write back the lines to the file
    file = $fopen(filename, "w");
    defer { fclose(file); };

    for (size_t i = 0; i < lines_count; i++) {
        fputs(lines[i], file);
    }

    printf("Deleted line %d from '%s' successfully.\n", line_number, filename);
    log_change("Delete Line", filename, (size_t)line_number, lines_count);
    return 0;
}

static int insert_line(size_t param_len, const char *nonnull params[static param_len])
{
    const char *filename = params[0];
    int line_number = atoi(params[1]);
    const char *line_content_raw = params[2];
    //we need a newline!
    char line_content[strlen(line_content_raw) + 2]; //I love VLAs :)
    snprintf(line_content, sizeof(line_content), "%s\n", line_content_raw);

    auto file = $fopen(filename, "r");
    defer { fclose(file); };

    // Read all lines into memory
    char *nonnull *nullable lines = nullptr;
    size_t lines_allocated = 0, lines_count = 0;
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), file)) {
        if (lines_count >= lines_allocated) {
            lines_allocated = lines_allocated ? lines_allocated * 2 : 10;
            lines = $realloc(lines, lines_allocated * sizeof(char *));
        }
        lines[lines_count] = $strdup(buffer);
        lines_count++;
    }

    //we cant defer because the C standard mandates that `realloc` can return the same pointer
    //therefore having a defer here and another defer after the realloc would cause 2 possible states, both are invalid:
    //1. the pointer remains the same, and the first defer would free the memory, causing the second defer to free invalid memory
    //2. the pointer changes, and the first defer would free a pointer that is invalidated (if `realloc` succeeds then the pointer passsed into it could be invalidated), causing UB
    if (line_number < 1 or (size_t)line_number > lines_count + 1) {
        fprintf(stderr, "Invalid line number.\n");
        for (size_t i = 0; i < lines_count; i++) {
            free(lines[i]);
        }
        free(lines);
        return 1;
    }

    lines = $realloc(lines, (lines_count + 1) * sizeof(char *));
    defer {
        for (size_t i = 0; i < lines_count + 1; i++) { //we add 1 because we add the new line content in next bit of code
            free(lines[i]);
        }
        free(lines);
    };
    for (size_t i = lines_count; i >= (size_t)(line_number); i--) {
        lines[i] = lines[i - 1];
    }
    lines[line_number - 1] = $strdup(line_content);
    lines_count++;

    // Write back to the file
    file = $fopen(filename, "w");
    defer { fclose(file); };

    for (size_t i = 0; i < lines_count; i++) {
        fputs(lines[i], file);
    }

    printf("Inserted line at %d in '%s' successfully.\n", line_number, filename);

    // Get the number of lines after insertion
    size_t total_lines = lines_count;

    log_change("Insert Line", filename, (size_t)line_number, total_lines);
    return 0;
}

static int show_line(size_t param_len, const char *nonnull params[static param_len])
{
    const char *filename = params[0];
    int line_number = atoi(params[1]);

    auto file = $fopen(filename, "r");
    defer { fclose(file); };

    char buffer[1024];
    size_t current_line = 1;
    while (fgets(buffer, sizeof(buffer), file)) {
        if (current_line == (size_t)line_number) {
            printf("Line %d: %s", line_number, buffer);
            return 0;
        }
        current_line++;
    }

    fprintf(stderr, "Line number %d does not exist in '%s'.\n", line_number, filename);
    return 1;
}


static int show_change_log(size_t, const char *nonnull[])
{
    if (changelog_count == 0) {
        printf("No operations have been performed yet.\n");
        return 0;
    }

    printf("Change Log:\n");
    for (size_t i = 0; i < changelog_count; i++) {
        struct ChangelogEntry *entry = &changelog[i];
        if (strcmp(entry->operation, "Append Line") == 0 or
            strcmp(entry->operation, "Insert Line") == 0 or
            strcmp(entry->operation, "Delete Line") == 0) {
            printf("%zu. %s on '%s' at line %zu. Total lines: %zu.\n",
                   i + 1, entry->operation, entry->filename,
                   entry->line_number, entry->total_lines);
        } else {
            printf("%zu. %s on '%s'. Total lines: %zu.\n",
                   i + 1, entry->operation, entry->filename,
                   entry->total_lines);
        }
    }
    return 0;
}

static int show_number_of_lines(size_t param_len, const char *nonnull params[static param_len])
{
    const char *filename = params[0];
    auto file = $fopen(filename, "r");
    defer { fclose(file); };

    size_t line_count = 0;
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), file)) {
        line_count++;
    }

    printf("File '%s' has %zu line(s).\n", filename, line_count);
    return 0;
}

static int help(size_t, const char *nonnull[])
{
    printf("Available commands:\n");
    for (size_t i = 0; i < command_count; ++i) {
        printf("  %s\n", commands[i].name);
    }
    return 0;
}


[[gnu::constructor(101)]]
void init_commands()
{
    // any commands added to the registry here is able to be picked up by the rest of the program and used.
    // registering it here will automatically make it available in `help` and executable from the commandline

    static struct Parameter create_file_params[] = {
        { .name = "filename", .optional = false, .type = ParameterType_STRING },
        {0} //every param array must end with a nullptr entry to indicate the end of the array
    };

    add_command((struct Command) {
        .name = "create-file",
        .action = &create_file,
        //C2x lets us have storage class specifiers for compound literals (https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3038.htm)
        //This has been implemented in GCC 13 but not clang yet, and I prefer clang because of the nullability extensions
        //so yucky variable instead :(
        .parameters = create_file_params
    });

    static struct Parameter copy_file_params[] = {
        { .name = "source", .optional = false, .type = ParameterType_STRING },
        { .name = "destination", .optional = false, .type = ParameterType_STRING },
        {0}
    };
    add_command((struct Command){
        .name = "copy-file",
        .action = &copy_file,
        .parameters = copy_file_params
    });

    static struct Parameter delete_file_params[] = {
        { .name = "filename", .optional = false, .type = ParameterType_STRING },
        {0}
    };
    add_command((struct Command){
        .name = "delete-file",
        .action = &delete_file,
        .parameters = delete_file_params
    });

    static struct Parameter show_file_params[] = {
        { .name = "filename", .optional = false, .type = ParameterType_STRING },
        {0}
    };
    add_command((struct Command){
        .name = "show-file",
        .action = &show_file,
        .parameters = show_file_params
    });

    static struct Parameter append_line_params[] = {
        { .name = "filename", .optional = false, .type = ParameterType_STRING },
        { .name = "line_content", .optional = false, .type = ParameterType_STRING },
        {0}
    };
    add_command((struct Command){
        .name = "append-line",
        .action = &append_line,
        .parameters = append_line_params
    });

    static struct Parameter delete_line_params[] = {
        { .name = "filename", .optional = false, .type = ParameterType_STRING },
        { .name = "line_number", .optional = false, .type = ParameterType_STRING }, // Using string to accept numeric input, im too lazy and tired to properly implement ParameterType_INTEGER
        {0}
    };
    add_command((struct Command){
        .name = "delete-line",
        .action = &delete_line,
        .parameters = delete_line_params
    });

    static struct Parameter insert_line_params[] = {
        { .name = "filename", .optional = false, .type = ParameterType_STRING },
        { .name = "line_number", .optional = false, .type = ParameterType_STRING },
        { .name = "line_content", .optional = false, .type = ParameterType_STRING },
        {0}
    };
    add_command((struct Command){
        .name = "insert-line",
        .action = &insert_line,
        .parameters = insert_line_params
    });

    static struct Parameter show_line_params[] = {
        { .name = "filename", .optional = false, .type = ParameterType_STRING },
        { .name = "line_number", .optional = false, .type = ParameterType_STRING },
        {0}
    };
    add_command((struct Command){
        .name = "show-line",
        .action = &show_line,
        .parameters = show_line_params
    });

    static struct Parameter show_change_log_params[] = {
        {0}
    };
    add_command((struct Command){
        .name = "changelog",
        .action = &show_change_log,
        .parameters = show_change_log_params
    });

    static struct Parameter show_number_of_lines_params[] = {
        { .name = "filename", .optional = false, .type = ParameterType_STRING },
        {0}
    };
    add_command((struct Command){
        .name = "line-count",
        .action = &show_number_of_lines,
        .parameters = show_number_of_lines_params
    });

    static struct Parameter help_params[] = {
        {0}
    };
    add_command((struct Command){
        .name = "help",
        .action = &help,
        .parameters = help_params
    });
}

void add_command(struct Command cmd)
{
    if (command_count < MAX_COMMANDS) {
        commands[command_count++] = cmd;
    } else {
        fprintf(stderr, "Maximum number of commands reached.\n");
        exit(1);
    }
}

struct Command *find_command(const char *name)
{
    for (size_t i = 0; i < command_count; ++i) {
        if (strcmp(commands[i].name, name) == 0) {
            return &commands[i];
        }
    }
    return nullptr;
}

#pragma clang assume_nonnull end
