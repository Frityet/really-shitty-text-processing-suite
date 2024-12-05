#include "commands.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma clang assume_nonnull begin

static struct Command commands[MAX_COMMANDS];
static size_t command_count = 0;

static inline void get_changelog_filename(const char *filename, char *changelog_filename, size_t size)
{ snprintf(changelog_filename, size, "%s.changelog", filename); }

static int parse_changelog(const char *filename, struct Changelog **changelog)
{
    char changelog_filename[PATH_MAX];
    get_changelog_filename(filename, changelog_filename, sizeof(changelog_filename));

    auto file = $fopen(changelog_filename, "rb");

    // Determine the file size
    if (fseek(file, 0, SEEK_END) != 0) {
        perror("Error seeking to end of changelog file");
        fclose(file);
        return -1;
    }
    long filesize = ftell(file);
    if (filesize < 0) {
        perror("Error determining changelog file size");
        fclose(file);
        return -1;
    }
    rewind(file);

    size_t num_entries = filesize / sizeof(struct ChangelogEntry);

    // Allocate memory for Changelog struct with enough space for entries
    struct Changelog *cl = $malloc(sizeof(struct Changelog) + num_entries * sizeof(struct ChangelogEntry));

    cl->length = num_entries;

    // Read the entries into the Changelog struct
    size_t read = fread(cl->entries, sizeof(struct ChangelogEntry), num_entries, file);
    if (read != num_entries) {
        perror("Error reading changelog entries");
        free(cl);
        fclose(file);
        return -1;
    }

    fclose(file);
    *changelog = cl;
    return 0;
}

static void log_change(const char *operation, const char *filename, size_t line_number, size_t total_lines)
{
    char changelog_filename[PATH_MAX];
    get_changelog_filename(filename, changelog_filename, sizeof(changelog_filename));

    auto changelog_file = $fopen(changelog_filename, "ab");

    struct ChangelogEntry entry = {0};
    strncpy(entry.operation, operation, sizeof(entry.operation) - 1);
    entry.operation[sizeof(entry.operation) - 1] = '\0';
    entry.timestamp = time(nullptr);
    entry.line_number = line_number;
    entry.total_lines = total_lines;

    if (fwrite(&entry, sizeof(entry), 1, changelog_file) != 1) {
        perror("Error writing to changelog file");
    }

    fclose(changelog_file);
}

static int create_file(size_t param_len, const char *nonnull params[static param_len])
{
    const char *filename = params[0];
    auto file = $fopen(filename, "wb");
    defer { fclose(file); };

    log_change("Create File", filename, 0, 0);
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
    
    log_change("Copy File", destination, 0, 0);
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
    log_change("Append Line", filename, total_lines, total_lines);
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


static int show_change_log(size_t param_len, const char *nonnull params[static param_len])
{
    const char *filename = params[0];

    struct Changelog *nonnull changelog;
    if (parse_changelog(filename, &changelog) != 0) {
        fprintf(stderr, "Failed to parse changelog for file '%s'.\n", filename);
        return 1;
    }

    printf("Change Log for '%s':\n", filename);

    for (size_t idx = 0; idx < changelog->length; idx++) {
        struct ChangelogEntry *entry = &changelog->entries[idx];

        char timestamp[32] = {0};
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&entry->timestamp));

        if (strcmp(entry->operation, "Append Line") == 0 ||
            strcmp(entry->operation, "Insert Line") == 0 ||
            strcmp(entry->operation, "Delete Line") == 0) {
            printf("%zu. %s at %s on line %zu. Total lines: %zu.\n",
                   idx + 1, entry->operation, timestamp,
                   entry->line_number, entry->total_lines);
        } else {
            printf("%zu. %s at %s. Total lines: %zu.\n",
                   idx + 1, entry->operation, timestamp,
                   entry->total_lines);
        }
    }

    free(changelog);
    return 0;
}

static int show_number_of_lines(size_t param_len, const char *nonnull params[static param_len])
{
    const char *filename = params[0];
    auto file = $fopen(filename, "r");
    defer { fclose(file); };

    size_t line_count = 0;
    char buffer[1024];

    //fgets reads until end of the line
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

static int find(size_t param_len, const char *nonnull params[static param_len])
{
    const char *filename = params[0], *search_string = params[1];

    auto file = $fopen(filename, "r");
    defer { fclose(file); };

    char buffer[1024];
    size_t line_number = 1, matches = 0;

    while (fgets(buffer, sizeof(buffer), file)) {
        if (strstr(buffer, search_string) != NULL) {
            printf("Line %zu: %s", line_number, buffer);
            matches++;
        }
        line_number++;
    }

    if (matches == 0) {
        printf("No matches found for '%s' in '%s'.\n", search_string, filename);
    } else {
        printf("Found %zu matching line(s) for '%s' in '%s'.\n", matches, search_string, filename);
    }

    return 0;
}

static int trim(size_t param_len, const char *nonnull params[static param_len])
{
    const char *filename = params[0];

    auto file = $fopen(filename, "r");

    // Read all lines into memory
    char *nonnull *nullable lines = nullptr;
    size_t lines_allocated = 0, lines_count = 0;
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), file)) {
        if (lines_count >= lines_allocated) {
            lines_allocated = lines_allocated ? lines_allocated * 2 : 10;
            lines = $realloc(lines, lines_allocated * sizeof(char *));
        }
        char *tmpbuf = $calloc(sizeof(buffer) + 1, sizeof(char)); //we need teh whole size of the buffer for when we add a newline later
        strcpy(tmpbuf, buffer);
        lines[lines_count] = tmpbuf;
        lines_count++;
    }
    fclose(file); // just in case its auto-locked, we don't want it to mess up our writes
    defer {
        for (size_t i = 0; i < lines_count; i++) {
            free(lines[i]);
        }
        free(lines);
    };

    for (size_t i = 0; i < lines_count; i++) {
        char *line = lines[i];
        size_t len = strlen(line);
        //removing all trailing whitespace
        while (len > 0 and (line[len - 1] == ' ' or line[len - 1] == '\t' or line[len - 1] == '\n' or line[len - 1] == '\r')) {
            line[len - 1] = '\0';
            len--;
        }

        // we need the newline (fuck u DOS line endings :))
        strcat(line, "\n");
    }

    file = $fopen(filename, "wb");
    defer { fclose(file); };

    for (size_t i = 0; i < lines_count; i++) {
        fputs(lines[i], file);
    }

    printf("Trimmed trailing whitespace from '%s' successfully.\n", filename);
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
        { .name = "filename", .optional = false, .type = ParameterType_STRING },
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

    //Additional feature #1: Trimming!
    //This command will remove all trailing whitespace from each line in a file
    //This saves space and makes editing files nicer and better
    static struct Parameter trim_params[] = {
        { .name = "filename", .optional = false, .type = ParameterType_STRING },
        {0}
    };
    add_command((struct Command){
        .name = "trim",
        .action = &trim,
        .parameters = trim_params
    });

    //Additional feature #2: Find!
    //This command will search for a string in a file and print out the lines that contain the string
    //Allows for easy searching of files
    static struct Parameter find_params[] = {
        { .name = "filename", .optional = false, .type = ParameterType_STRING },
        { .name = "search_string", .optional = false, .type = ParameterType_STRING },
        {0}
    };
    add_command((struct Command){
        .name = "find",
        .action = &find,
        .parameters = find_params
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

//Looks through a command in the registry
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
