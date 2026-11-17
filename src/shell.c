
#include "shell.h"

/* Command history storage */
static char* command_history[HISTORY_SIZE] = {NULL};
static int history_count = 0;

/* Read a command from input */
char* read_cmd(char* prompt, FILE* fp) {
    printf("%s", prompt);
    fflush(stdout);

    char* cmdline = malloc(sizeof(char) * MAX_LEN);

    if (cmdline == NULL) {
        perror("malloc failed");
        return NULL;
    }

    int c, pos = 0;

    while ((c = getc(fp)) != EOF) {
        if (c == '\n')
            break;

        if (pos < MAX_LEN - 1)
            cmdline[pos++] = c;
    }

    if (c == EOF && pos == 0) {
        free(cmdline);
        return NULL;
    }

    cmdline[pos] = '\0';
    return cmdline;
}

/* Tokenize the command line */
char** tokenize(char* cmdline) {

    if (cmdline == NULL || cmdline[0] == '\0')
        return NULL;

    char** arglist = malloc(sizeof(char*) * (MAXARGS + 1));

    if (arglist == NULL) {
        perror("malloc failed");
        return NULL;
    }

    for (int i = 0; i < MAXARGS + 1; i++) {
        arglist[i] = calloc(ARGLEN, sizeof(char));

        if (arglist[i] == NULL) {
            perror("malloc failed");

            for (int j = 0; j < i; j++)
                free(arglist[j]);

            free(arglist);
            return NULL;
        }
    }

    char* cp = cmdline;
    int argnum = 0;

    while (*cp != '\0' && argnum < MAXARGS) {

        while (*cp == ' ' || *cp == '\t')
            cp++;

        if (*cp == '\0')
            break;

        char* start = cp;
        int len = 0;

        while (*cp != '\0' && *cp != ' ' && *cp != '\t') {
            cp++;
            len++;
        }

        if (len >= ARGLEN) {
            fprintf(stderr, "Argument too long (maximum %d characters).\n",
                    ARGLEN - 1);

            for (int i = 0; i < MAXARGS + 1; i++)
                free(arglist[i]);

            free(arglist);
            return NULL;
        }

        strncpy(arglist[argnum], start, len);
        argnum++;
    }

    if (argnum == 0) {
        for (int i = 0; i < MAXARGS + 1; i++)
            free(arglist[i]);

        free(arglist);
        return NULL;
    }

    arglist[argnum] = NULL;
    return arglist;
}

/* Add a command to history */
void add_history(const char* command) {

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

