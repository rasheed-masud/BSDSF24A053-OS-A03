

#include "shell.h"

#define MAX_BLOCK_LINES 100

/* Remove leading and trailing whitespace */
static char* trim_whitespace(char* text) {
    while (*text == ' ' || *text == '\t' ||
           *text == '\n' || *text == '\r')
        text++;

    size_t len = strlen(text);

    while (len > 0 &&
           (text[len - 1] == ' ' ||
            text[len - 1] == '\t' ||
            text[len - 1] == '\n' ||
            text[len - 1] == '\r')) {
        text[--len] = '\0';
    }

    return text;
}

/* Execute all commands stored in a conditional block */
static void execute_block(char* lines[], int count) {
    for (int i = 0; i < count; i++) {
        char** args = tokenize(lines[i]);

        if (args != NULL) {
            execute(args);

            for (int j = 0; args[j] != NULL; j++)
                free(args[j]);

            free(args);
        }
    }
}

/* Read and execute a multi-line if-then-else-fi structure */
static void handle_if_statement(char* first_line) {
    char* condition = trim_whitespace(first_line + 2);

    if (*condition == '\0') {
        fprintf(stderr, "Syntax error: missing if condition\n");
        return;
    }

    char* then_lines[MAX_BLOCK_LINES];
    char* else_lines[MAX_BLOCK_LINES];

    int then_count = 0;
    int else_count = 0;

    int then_seen = 0;
    int else_seen = 0;
    int fi_seen = 0;

    while (!fi_seen) {
        check_background_jobs();

        char* input = readline("IF> ");

        if (input == NULL) {
            fprintf(stderr, "\nSyntax error: missing fi\n");
            break;
        }

        char* line = trim_whitespace(input);

        if (strcmp(line, "then") == 0) {
            if (then_seen || else_seen) {
                fprintf(stderr, "Syntax error: unexpected then\n");
                free(input);
                break;
            }

            then_seen = 1;
            free(input);
            continue;
        }

        if (strcmp(line, "else") == 0) {
            if (!then_seen || else_seen) {
                fprintf(stderr, "Syntax error: unexpected else\n");
                free(input);
                break;
            }

            else_seen = 1;
            free(input);
            continue;
        }

        if (strcmp(line, "fi") == 0) {
            if (!then_seen) {
                fprintf(stderr, "Syntax error: missing then\n");
                free(input);
                break;
            }

            fi_seen = 1;
            free(input);
            break;
        }

        if (!then_seen) {
            fprintf(stderr, "Syntax error: expected then\n");
            free(input);
            break;
        }

        if (!else_seen) {
            if (then_count >= MAX_BLOCK_LINES) {
                fprintf(stderr, "Then block is too large\n");
                free(input);
                break;
            }

            then_lines[then_count++] = strdup(line);
        } else {
            if (else_count >= MAX_BLOCK_LINES) {
                fprintf(stderr, "Else block is too large\n");
                free(input);
                break;
            }

            else_lines[else_count++] = strdup(line);
        }

        free(input);
    }

    if (fi_seen) {
        char** condition_args = tokenize(condition);

        if (condition_args == NULL) {
            fprintf(stderr, "Invalid if condition\n");
        } else {
            int status = execute_status(condition_args);

            for (int i = 0; condition_args[i] != NULL; i++)
                free(condition_args[i]);

            free(condition_args);

            if (status == 0) {
                execute_block(then_lines, then_count);
            } else if (else_seen) {
                execute_block(else_lines, else_count);
            }
        }
    }

    for (int i = 0; i < then_count; i++)
        free(then_lines[i]);

    for (int i = 0; i < else_count; i++)
        free(else_lines[i]);
}

int main(void) {
    char* cmdline;
    char** arglist;

    while (1) {
        check_background_jobs();

        cmdline = read_cmd(PROMPT, stdin);

        if (cmdline == NULL)
            break;

        char* trimmed = trim_whitespace(cmdline);

        if (*trimmed == '\0') {
            free(cmdline);
            continue;
        }

        /* Handle multi-line if statements */
        if (strncmp(trimmed, "if ", 3) == 0 ||
            strncmp(trimmed, "if\t", 3) == 0) {

            save_command_history(trimmed);
            handle_if_statement(trimmed);
            free(cmdline);
            continue;
        }

        /* Handle history display */
        if (strcmp(trimmed, "history") == 0) {
            print_history();
            free(cmdline);
            continue;
        }

        /* Handle !n command execution */
        if (trimmed[0] == '!') {
            char* endptr;
            long number = strtol(trimmed + 1, &endptr, 10);

            if (trimmed[1] == '\0' ||
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

            char* expanded = strdup(previous);

            if (expanded == NULL) {
                perror("strdup");
                free(cmdline);
                continue;
            }

            free(cmdline);
            cmdline = expanded;
            trimmed = cmdline;
        }

        save_command_history(trimmed);

        arglist = tokenize(trimmed);

        if (arglist != NULL) {
            execute(arglist);

            for (int i = 0; arglist[i] != NULL; i++)
                free(arglist[i]);

            free(arglist);
        }

        free(cmdline);
    }

    check_background_jobs();

    printf("\nShell exited.\n");
    return 0;
}
