#include "shell.h"

int main() {

    char* cmdline;
    char** arglist;

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {

        /* Skip empty commands */
        if (cmdline[0] == '\0') {
            free(cmdline);
            continue;
        }

        /* Handle history display */
        if (strcmp(cmdline, "history") == 0) {
            print_history();
            free(cmdline);
            continue;
        }

        /* Handle !n command execution */
        if (cmdline[0] == '!') {

            char* endptr;
            long number = strtol(cmdline + 1, &endptr, 10);

            if (cmdline[1] == '\0' ||
                *endptr != '\0' ||
                number < 1 ||
                number > 2147483647) {

                printf("Invalid history command.\n");
                free(cmdline);
                continue;
            }

            const char* previous = get_history((int)number);

            if (previous == NULL) {
                printf("No such command in history.\n");
                free(cmdline);
                continue;
            }

            printf("%s\n", previous);

            char* expanded = malloc(strlen(previous) + 1);

            if (expanded == NULL) {
                perror("malloc failed");
                free(cmdline);
                continue;
            }

            strcpy(expanded, previous);
            free(cmdline);
            cmdline = expanded;
        }

        /* Save command in history */
        save_command_history(cmdline);

        /* Tokenize and execute */
        arglist = tokenize(cmdline);

        if (arglist != NULL) {

            execute(arglist);

            for (int i = 0; arglist[i] != NULL; i++) {
                free(arglist[i]);
            }

            free(arglist);
        }

        free(cmdline);
    }

    printf("\nShell exited.\n");

    return 0;
}
