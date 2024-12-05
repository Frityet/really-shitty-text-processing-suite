#pragma once

#include "common.h"

#pragma clang assume_nonnull begin

typedef int Action_f(size_t param_len, const char *nonnull params[]);

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

enum {
    MAX_COMMANDS = 128,
    MAX_CHANGELOG_ENTRIES = 1024,
};



void add_command(struct Command cmd);
struct Command *nullable find_command(const char *name);

#pragma clang assume_nonnull end
