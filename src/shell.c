
#include "shell.h"

/* Command history storage */
static char* command_history[HISTORY_SIZE] = {NULL};
static int history_count = 0;

/* Shell variable storage */
static ShellVariable* variable_list = NULL;

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

/* Check whether a character is valid in a variable name */
static int is_valid_variable_char(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
           c == '_';
}

/* Validate a variable name */
static int is_valid_variable_name(const char* name) {
    if (name == NULL || name[0] == '\0')
        return 0;

    if (!((name[0] >= 'a' && name[0] <= 'z') ||
          (name[0] >= 'A' && name[0] <= 'Z') ||
          name[0] == '_'))
        return 0;

    for (int i = 1; name[i] != '\0'; i++) {
        if (!is_valid_variable_char(name[i]))
            return 0;
    }

    return 1;
}

/* Find a variable by name */
static ShellVariable* find_variable(const char* name) {
    ShellVariable* current = variable_list;

    while (current != NULL) {
        if (strcmp(current->name, name) == 0)
            return current;

        current = current->next;
    }

    return NULL;
}

/* Check whether a token is a variable assignment */
int is_variable_assignment(const char* token) {
    if (token == NULL)
        return 0;

    const char* equals = strchr(token, '=');

    if (equals == NULL || equals == token)
        return 0;

    size_t name_length = (size_t)(equals - token);
    char* name = malloc(name_length + 1);

    if (name == NULL)
        return 0;

    memcpy(name, token, name_length);
    name[name_length] = '\0';

    int valid = is_valid_variable_name(name);

    free(name);
    return valid;
}

/* Assign or update a shell variable */
int assign_variable(const char* assignment) {
    if (!is_variable_assignment(assignment)) {
        fprintf(stderr, "Invalid variable assignment.\n");
        return 1;
    }

    const char* equals = strchr(assignment, '=');
    size_t name_length = (size_t)(equals - assignment);

    char* name = malloc(name_length + 1);

    if (name == NULL) {
        perror("variable name allocation failed");
        return 1;
    }

    memcpy(name, assignment, name_length);
    name[name_length] = '\0';

    char* value = strdup(equals + 1);

    if (value == NULL) {
        perror("variable value allocation failed");
        free(name);
        return 1;
    }

    ShellVariable* variable = find_variable(name);

    if (variable != NULL) {
        free(variable->value);
        variable->value = value;
        free(name);
        return 0;
    }

    variable = malloc(sizeof(ShellVariable));

    if (variable == NULL) {
        perror("variable allocation failed");
        free(name);
        free(value);
        return 1;
    }

    variable->name = name;
    variable->value = value;
    variable->next = variable_list;
    variable_list = variable;

    return 0;
}

/* Expand arguments that consist of $VARIABLE */
void expand_variables(char** arglist) {
    if (arglist == NULL)
        return;

    for (int i = 0; arglist[i] != NULL; i++) {
        char* argument = arglist[i];

        if (argument[0] != '$' || argument[1] == '\0')
            continue;

        const char* name = argument + 1;

        if (!is_valid_variable_name(name))
            continue;

        ShellVariable* variable = find_variable(name);

        /* Undefined variables expand to an empty string */
        const char* value = variable != NULL ? variable->value : "";

        char* expanded = strdup(value);

        if (expanded == NULL) {
            perror("variable expansion failed");
            continue;
        }

        free(arglist[i]);
        arglist[i] = expanded;
    }
}

/* Display all defined variables */
void print_variables(void) {
    if (variable_list == NULL) {
        printf("No variables defined.\n");
        return;
    }

    ShellVariable* current = variable_list;

    while (current != NULL) {
        printf("%s=%s\n", current->name, current->value);
        current = current->next;
    }
}

/* Free all shell variables */
void free_variables(void) {
    ShellVariable* current = variable_list;

    while (current != NULL) {
        ShellVariable* next = current->next;

        free(current->name);
        free(current->value);
        free(current);

        current = next;
    }

    variable_list = NULL;
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
