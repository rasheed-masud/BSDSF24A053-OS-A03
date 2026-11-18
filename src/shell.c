#include "shell.h"

/* Command history storage */
static char* command_history[HISTORY_SIZE] = {NULL};
static int history_count = 0;

/* Read a command from input */
char* read_cmd(char* prompt, FILE* fp) {
    (void)fp;
    return readline(prompt);
}

/* Free an allocated argument list */
static void free_arglist(char** arglist) {
    if (arglist == NULL)
        return;

    for (int i = 0; arglist[i] != NULL; i++)
        free(arglist[i]);

    free(arglist);
}

/* Tokenize commands and recognize shell operators */
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
            fprintf(stderr, "Too many arguments (maximum %d).\n", MAXARGS);
            free_arglist(arglist);
            return NULL;
        }

        /* Recognize shell operators */
        if (*cp == '|' || *cp == ';' || *cp == '&') {
    char op[2] = {*cp, '\0'};
    arglist[argnum] = strdup(op);
    cp++;
} else if (*cp == '<') {
            arglist[argnum] = strdup("<");
            cp++;
        } else if (*cp == '>') {
            if (*(cp + 1) == '>') {
                arglist[argnum] = strdup(">>");
                cp += 2;
            } else {
                arglist[argnum] = strdup(">");
                cp++;
            }
        } else {
            char* start = cp;

            while (*cp != '\0' &&
                   *cp != ' ' && *cp != '\t' &&
                   *cp != '|' && *cp != '<' && *cp != '>')
                cp++;

            size_t len = (size_t)(cp - start);

            if (len >= ARGLEN) {
                fprintf(stderr, "Argument too long (maximum %d characters).\n",
                        ARGLEN - 1);
                free_arglist(arglist);
                return NULL;
            }

            arglist[argnum] = malloc(len + 1);

            if (arglist[argnum] == NULL) {
                perror("malloc failed");
                free_arglist(arglist);
                return NULL;
            }

            memcpy(arglist[argnum], start, len);
            arglist[argnum][len] = '\0';
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

/* Add a command to history */
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
