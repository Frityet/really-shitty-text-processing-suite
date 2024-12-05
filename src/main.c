#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "commands.h"

int main(int argc, const char *argv[])
{

    if (argc < 2) {
        fprintf(stderr, "No command specified.\n");
        return 1;
    }

    const char *command_name = argv[1];
    struct Command *cmd = find_command(command_name);
    if (cmd == nullptr) {
        fprintf(stderr, "Command '%s' not found.\n", command_name);
        return 1;
    }

    size_t expected_params = 0;
    while (cmd->parameters[expected_params].name) {
        expected_params++;
    }

    if ((argc - 2) < (int)expected_params) {
        fprintf(stderr, "Insufficient parameters for command '%s'.\n", command_name);
        return 1;
    }

    return cmd->action(argc - 2, &argv[2]);
}
