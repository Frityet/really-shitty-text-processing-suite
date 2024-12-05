#pragma once

#include "common.h"
//fuck you linux! Just be posix compliant!!
#if defined(__linux__)
#   include <linux/limits.h>
#else
#   include <limits.h>
#endif


#pragma clang assume_nonnull begin

typedef int Action_f(size_t param_len, const char *nonnull params[nonnull]);

struct Command {
    const char *name;
    Action_f *action;
    struct Parameter {
        const char *name;
        bool optional;
        enum ParameterType {
            ParameterType_STRING,
            ParameterType_INTEGER
        } type;
    } *parameters;
};

struct ChangelogEntry {
    char    operation[64],      //This should be dynamic, but at the same time I don't care!
            filename[PATH_MAX];
    size_t  line_number,        // For line operations
            total_lines;        // Number of lines after operation
};

enum {
    MAX_COMMANDS = 128,
    MAX_CHANGELOG_ENTRIES = 1024,
};


void add_command(struct Command cmd);
struct Command *nullable find_command(const char *name);
const struct ChangelogEntry *nullable get_changelog_entry(size_t idx);
size_t get_changelog_count(void);

#pragma clang assume_nonnull end
