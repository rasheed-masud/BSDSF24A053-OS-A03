
#include "shell.h"

/* Maximum number of commands in one pipeline */
#define MAX_PIPE_COMMANDS MAXARGS

typedef struct {
    char* argv[MAXARGS + 1];
    char* input_file;
    char* output_file;
    int append;
} Command;

/* Check whether a token is a shell operator */
static int is_operator(const char* token) {
    return token != NULL &&
           (strcmp(token, "|") == 0 ||
            strcmp(token, "<") == 0 ||
            strcmp(token, ">") == 0 ||
            strcmp(token, ">>") == 0);
}

/* Check whether a command is a shell built-in */
int is_builtin(char* command) {
    if (command == NULL)
        return 0;

    return (
        strcmp(command, "cd") == 0 ||
        strcmp(command, "exit") == 0 ||
        strcmp(command, "help") == 0 ||
        strcmp(command, "jobs") == 0
    );
}

/* Display available shell commands */
void display_help(void) {
    printf("\n");
    printf("========== FCIT CUSTOM SHELL ==========\n");
    printf("Built-in commands:\n");
    printf("  cd [directory]  Change directory\n");
    printf("  exit            Exit the shell\n");
    printf("  help            Display this help message\n");
    printf("  jobs            Display background jobs\n");
    printf("\nRedirection:\n");
    printf("  command < file  Input redirection\n");
    printf("  command > file  Output redirection\n");
    printf("  command >> file Append output\n");
    printf("\nPipelines:\n");
    printf("  command1 | command2\n");
    printf("=======================================\n\n");
}

/* Display background jobs */
void display_jobs(void) {
    printf("No background jobs to display.\n");
}

/* Execute built-in commands */
int execute_builtin(char** arglist) {
    if (strcmp(arglist[0], "cd") == 0) {
        char* directory;

        if (arglist[1] == NULL) {
            directory = getenv("HOME");

            if (directory == NULL) {
                fprintf(stderr, "cd: HOME is not set\n");
                return 1;
            }
        } else {
            directory = arglist[1];
        }

        if (chdir(directory) != 0)
            perror("cd");

        return 1;
    }

    if (strcmp(arglist[0], "exit") == 0)
        exit(0);

    if (strcmp(arglist[0], "help") == 0) {
        display_help();
        return 1;
    }

    if (strcmp(arglist[0], "jobs") == 0) {
        display_jobs();
        return 1;
    }

    return 0;
}

/* Parse one command between pipeline separators */
static int parse_command(char** arglist, int start, int end,
                         Command* command) {
    int argc = 0;

    memset(command, 0, sizeof(Command));

    for (int i = start; i < end; i++) {
        if (strcmp(arglist[i], "<") == 0 ||
            strcmp(arglist[i], ">") == 0 ||
            strcmp(arglist[i], ">>") == 0) {

            char* op = arglist[i];

            if (i + 1 >= end || is_operator(arglist[i + 1])) {
                fprintf(stderr, "Syntax error: missing filename after %s\n",
                        op);
                return -1;
            }

            if (strcmp(op, "<") == 0) {
                if (command->input_file != NULL) {
                    fprintf(stderr, "Multiple input redirections\n");
                    return -1;
                }
                command->input_file = arglist[++i];
            } else {
                if (command->output_file != NULL) {
                    fprintf(stderr, "Multiple output redirections\n");
                    return -1;
                }
                command->output_file = arglist[++i];
                command->append = (strcmp(op, ">>") == 0);
            }
        } else {
            if (argc >= MAXARGS) {
                fprintf(stderr, "Too many arguments\n");
                return -1;
            }

            command->argv[argc++] = arglist[i];
        }
    }

    command->argv[argc] = NULL;

    if (argc == 0) {
        fprintf(stderr, "Syntax error: empty command\n");
        return -1;
    }

    return 0;
}

/* Execute commands with pipes and redirection */
int execute(char** arglist) {
    if (arglist == NULL || arglist[0] == NULL)
        return 0;

    Command commands[MAX_PIPE_COMMANDS];
    pid_t pids[MAX_PIPE_COMMANDS];
    int pipes[MAX_PIPE_COMMANDS - 1][2];

    int command_count = 0;
    int start = 0;

    /* Split the token list into pipeline commands */
    for (int i = 0; ; i++) {
        if (arglist[i] == NULL || strcmp(arglist[i], "|") == 0) {
            if (command_count >= MAX_PIPE_COMMANDS) {
                fprintf(stderr, "Too many pipeline commands\n");
                return -1;
            }

            if (parse_command(arglist, start, i,
                              &commands[command_count]) != 0)
                return -1;

            command_count++;
            start = i + 1;

            if (arglist[i] == NULL)
                break;
        }
    }

    /* Preserve parent-process behavior for simple built-ins */
    if (command_count == 1 &&
        is_builtin(commands[0].argv[0]) &&
        commands[0].input_file == NULL &&
        commands[0].output_file == NULL) {
        execute_builtin(commands[0].argv);
        return 0;
    }

    /* Create all required pipes */
    for (int i = 0; i < command_count - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe failed");

            for (int j = 0; j < i; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            return -1;
        }
    }

    /* Create a child process for each command */
    int launched = 0;

    for (int i = 0; i < command_count; i++) {
        pids[i] = fork();

        if (pids[i] == -1) {
            perror("fork failed");
            break;
        }

        if (pids[i] == 0) {
            /* Connect input to the previous pipe */
            if (i > 0) {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1) {
                    perror("dup2 input pipe");
                    _exit(EXIT_FAILURE);
                }
            }

            /* Connect output to the next pipe */
            if (i < command_count - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    perror("dup2 output pipe");
                    _exit(EXIT_FAILURE);
                }
            }

            /* Close all pipe descriptors in the child */
            for (int j = 0; j < command_count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            /* Handle input redirection */
            if (commands[i].input_file != NULL) {
                int fd = open(commands[i].input_file, O_RDONLY);

                if (fd == -1) {
                    perror(commands[i].input_file);
                    _exit(EXIT_FAILURE);
                }

                if (dup2(fd, STDIN_FILENO) == -1) {
                    perror("dup2 input file");
                    close(fd);
                    _exit(EXIT_FAILURE);
                }

                close(fd);
            }

            /* Handle output redirection */
            if (commands[i].output_file != NULL) {
                int flags = O_WRONLY | O_CREAT;

                if (commands[i].append)
                    flags |= O_APPEND;
                else
                    flags |= O_TRUNC;

                int fd = open(commands[i].output_file, flags, 0644);

                if (fd == -1) {
                    perror(commands[i].output_file);
                    _exit(EXIT_FAILURE);
                }

                if (dup2(fd, STDOUT_FILENO) == -1) {
                    perror("dup2 output file");
                    close(fd);
                    _exit(EXIT_FAILURE);
                }

                close(fd);
            }

            /* Execute built-ins inside pipeline children */
            if (is_builtin(commands[i].argv[0])) {
                execute_builtin(commands[i].argv);
                _exit(EXIT_SUCCESS);
            }

            execvp(commands[i].argv[0], commands[i].argv);
            perror(commands[i].argv[0]);
            _exit(EXIT_FAILURE);
        }

        launched++;
    }

    /* Close pipe descriptors in the parent */
    for (int i = 0; i < command_count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    /* Wait for all successfully created children */
    for (int i = 0; i < launched; i++) {
        int status;

        if (waitpid(pids[i], &status, 0) == -1)
            perror("waitpid failed");
    }

    return launched == command_count ? 0 : -1;
}
