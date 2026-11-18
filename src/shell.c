
#include "shell.h"

/* Command history storage */
static char* command_history[HISTORY_SIZE] = {NULL};
static int history_count = 0;

/* Read a command */
char* read_cmd(char* prompt, FILE* fp) {
    (void)fp;
    return readline(prompt);
}

/* Free an argument list */
static void free_arglist(char** arglist) {
    if (arglist == NULL)
        return;

    for (int i = 0; arglist[i] != NULL; i++)
        free(arglist[i]);

    free(arglist);
}

/* Check shell operators */
static int is_shell_operator(char c) {
    return c == '|' || c == '<' || c == '>' ||
           c == ';' || c == '&';
}

/* Tokenize commands, supporting quotes and shell operators */
char** tokenize(char* cmdline) {
    if (cmdline == NULL || cmdline[0] == '\0')
        return NULL;

    char** arglist = calloc(MAXARGS + 1, sizeof(char*));

    if (arglist == NULL) {
        perror("malloc failed");
        return NULL;
    }

    int argnum = 0;
    char* cp = cmdline;

    while (*cp != '\0') {
        while (*cp == ' ' || *cp == '\t')
            cp++;

        if (*cp == '\0')
            break;

        if (argnum >= MAXARGS) {
            fprintf(stderr, "Too many arguments (maximum %d).\n",
                    MAXARGS);
            free_arglist(arglist);
            return NULL;
        }

        /* Recognize operators outside quoted strings */
        if (is_shell_operator(*cp)) {
            char op[3] = {*cp, '\0', '\0'};

            if (*cp == '>' && *(cp + 1) == '>') {
                op[1] = '>';
                cp += 2;
            } else {
                cp++;
            }

            arglist[argnum] = strdup(op);
        } else {
            char token[ARGLEN];
            int length = 0;
            char quote = '\0';

            while (*cp != '\0') {
                if (quote == '\0' &&
                    (*cp == ' ' || *cp == '\t' ||
                     is_shell_operator(*cp)))
                    break;

                if ((*cp == '"' || *cp == '\'') &&
                    (quote == '\0' || quote == *cp)) {
                    if (quote == '\0')
                        quote = *cp;
                    else
                        quote = '\0';

                    cp++;
                    continue;
                }

                if (length >= ARGLEN - 1) {
                    fprintf(stderr,
                            "Argument too long (maximum %d characters).\n",
                            ARGLEN - 1);
                    free_arglist(arglist);
                    return NULL;
                }

                token[length++] = *cp++;
            }

            if (quote != '\0') {
                fprintf(stderr, "Syntax error: unmatched quotation mark\n");
                free_arglist(arglist);
                return NULL;
            }

            token[length] = '\0';
            arglist[argnum] = strdup(token);
        }

        if (arglist[argnum] == NULL) {
            perror("token allocation failed");
            free_arglist(arglist);
            return NULL;
        }

        argnum++;
    }

    if (argnum == 0) {
        free_arglist(arglist);
        return NULL;
    }

    arglist[argnum] = NULL;
    return arglist;
}

/* Save a command in the history */
void save_command_history(const char* command) {
    if (command == NULL || command[0] == '\0')
        return;

    int index = history_count % HISTORY_SIZE;

    char* copy = malloc(strlen(command) + 1);

    if (copy == NULL) {
        perror("history allocation failed");
        return;
    }

    strcpy(copy, command);

    free(command_history[index]);
    command_history[index] = copy;

    history_count++;
    add_history(command);
}

/* Display command history */
void print_history(void) {
    int start = history_count - HISTORY_SIZE;

    if (start < 0)
        start = 0;

    if (history_count == 0) {
        printf("No commands in history.\n");
        return;
    }

    for (int i = start; i < history_count; i++) {
        int index = i % HISTORY_SIZE;
        printf("%d  %s\n", i + 1, command_history[index]);
    }
}

/* Retrieve a command from history */
const char* get_history(int command_number) {
    if (command_number < 1 || command_number > history_count)
        return NULL;

    if (command_number <= history_count - HISTORY_SIZE)
        return NULL;

    return command_history[(command_number - 1) % HISTORY_SIZE];
}
