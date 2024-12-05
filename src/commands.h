#pragma once

#include "common.h"

#include <time.h>
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
    char    operation[64];      //This should be dynamic, but at the same time I don't care!
    time_t timestamp;
    size_t  line_number,        // For line operations
            total_lines;        // Number of lines after operation
};

struct Changelog {
    size_t length;
    struct ChangelogEntry entries[];
};

enum {
    MAX_COMMANDS = 128,
};


void add_command(struct Command cmd);
struct Command *nullable find_command(const char *name);

#pragma clang assume_nonnull end
