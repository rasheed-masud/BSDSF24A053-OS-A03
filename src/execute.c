
#include "shell.h"

#define MAX_PIPE_COMMANDS MAXARGS

typedef struct {
    char* argv[MAXARGS + 1];
    char* input_file;
    char* output_file;
    int append;
} Command;

typedef struct {
    pid_t pid;
    int job_id;
    char command[MAX_LEN];
    int active;
} BackgroundJob;

static BackgroundJob job_table[MAX_JOBS];
static int next_job_id = 1;
static int last_command_status = 0;

/* Check shell operators */
static int is_operator(const char* token) {
    return token != NULL &&
           (strcmp(token, "|") == 0 ||
            strcmp(token, "<") == 0 ||
            strcmp(token, ">") == 0 ||
            strcmp(token, ">>") == 0);
}

/* Check built-in commands */
int is_builtin(char* command) {
    if (command == NULL)
        return 0;

    return strcmp(command, "cd") == 0 ||
           strcmp(command, "exit") == 0 ||
           strcmp(command, "help") == 0 ||
           strcmp(command, "jobs") == 0 ||
           strcmp(command, "set") == 0;
}

/* Display help */

/* Display help */
void display_help(void) {
    printf("\n");
    printf("========== FCIT CUSTOM SHELL ==========\n");

    printf("Built-in commands:\n");
    printf("  cd [directory]  Change directory\n");
    printf("  exit            Exit the shell\n");
    printf("  help            Display help\n");
    printf("  jobs            Display background jobs\n");
    printf("  set             Display shell variables\n");

    printf("\nVariable assignment:\n");
    printf("  NAME=value      Create or update a variable\n");
    printf("  $NAME           Expand a variable\n");

    printf("\nVariable usage example:\n");
    printf("  MSG=Hello       Assign a value to MSG\n");
    printf("  echo $MSG       Display the variable value\n");
    printf("  set             List all defined variables\n");

    printf("\nRedirection:\n");
    printf("  command < file  Input redirection\n");
    printf("  command > file  Output redirection\n");
    printf("  command >> file Append output\n");

    printf("\nPipelines:\n");
    printf("  command1 | command2\n");

    printf("\nMultitasking:\n");
    printf("  command1 ; command2\n");
    printf("  command &\n");

    printf("\nConditional execution:\n");
    printf("  if condition\n");
    printf("  then\n");
    printf("      commands\n");
    printf("  else\n");
    printf("      commands\n");
    printf("  fi\n");

    printf("=======================================\n\n");
}
/* Add a background process */
void add_background_job(pid_t pid, const char* command) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!job_table[i].active) {
            job_table[i].pid = pid;
            job_table[i].job_id = next_job_id++;
            job_table[i].active = 1;

            snprintf(job_table[i].command,
                     sizeof(job_table[i].command),
                     "%s",
                     command != NULL ? command : "background process");

            printf("[%d] %d\n",
                   job_table[i].job_id, (int)pid);
            return;
        }
    }

    fprintf(stderr, "Job table is full. Cannot track PID %d\n",
            (int)pid);
}

/* Check background jobs without blocking */
void check_background_jobs(void) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!job_table[i].active)
            continue;

        int status;
        pid_t result = waitpid(job_table[i].pid, &status, WNOHANG);

        if (result == job_table[i].pid) {
            printf("[%d] Done\t%s\n",
                   job_table[i].job_id,
                   job_table[i].command);

            job_table[i].active = 0;
        } else if (result == -1 && errno == ECHILD) {
            job_table[i].active = 0;
        } else if (result == -1) {
            perror("waitpid");
        }
    }
}

/* Display background jobs */
void print_background_jobs(void) {
    check_background_jobs();

    int found = 0;

    for (int i = 0; i < MAX_JOBS; i++) {
        if (job_table[i].active) {
            printf("[%d] Running\t%s\n",
                   job_table[i].job_id,
                   job_table[i].command);
            found = 1;
        }
    }

    if (!found)
        printf("No background jobs are currently running.\n");
}

void display_jobs(void) {
    print_background_jobs();
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

        if (chdir(directory) != 0) {
            perror("cd");
            return 1;
        }

        return 1;
    }

    if (strcmp(arglist[0], "exit") == 0) {
        free_variables();
        exit(0);
    }

    if (strcmp(arglist[0], "help") == 0) {
        display_help();
        return 1;
    }

    if (strcmp(arglist[0], "jobs") == 0) {
        display_jobs();
        return 1;
    }

    if (strcmp(arglist[0], "set") == 0) {
        print_variables();
        return 1;
    }

    return 0;
}

/* Parse one command in a pipeline */
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
                fprintf(stderr,
                        "Syntax error: missing filename after %s\n", op);
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

/* Execute a pipeline and preserve its exit status */
static int execute_pipeline(char** arglist, int background,
                            const char* command_text) {
    if (arglist == NULL || arglist[0] == NULL)
        return 0;

    Command commands[MAX_PIPE_COMMANDS];
    pid_t pids[MAX_PIPE_COMMANDS];
    int pipes[MAX_PIPE_COMMANDS - 1][2];

    int command_count = 0;
    int start = 0;

    /* Split pipeline commands */
    for (int i = 0; ; i++) {
        if (arglist[i] == NULL ||
            strcmp(arglist[i], "|") == 0) {

            if (command_count >= MAX_PIPE_COMMANDS) {
                fprintf(stderr, "Too many pipeline commands\n");
                return 1;
            }

            if (parse_command(arglist, start, i,
                              &commands[command_count]) != 0)
                return 1;

            command_count++;
            start = i + 1;

            if (arglist[i] == NULL)
                break;
        }
    }

    /* Handle a standalone variable assignment in the parent */
    if (command_count == 1 &&
        commands[0].argv[1] == NULL &&
        commands[0].input_file == NULL &&
        commands[0].output_file == NULL &&
        is_variable_assignment(commands[0].argv[0])) {

        if (background) {
            fprintf(stderr,
                    "Variable assignments cannot run in the background.\n");
            return 1;
        }

        return assign_variable(commands[0].argv[0]);
    }

    /* Handle a simple built-in in the parent */
    if (command_count == 1 &&
        is_builtin(commands[0].argv[0]) &&
        commands[0].input_file == NULL &&
        commands[0].output_file == NULL) {

        if (background) {
            fprintf(stderr,
                    "Background execution of built-ins is not supported.\n");
            return 1;
        }

        execute_builtin(commands[0].argv);
        return 0;
    }

    /* Create pipes */
    for (int i = 0; i < command_count - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe failed");

            for (int j = 0; j < i; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            return 1;
        }
    }

    int launched = 0;

    /* Fork child processes */
    for (int i = 0; i < command_count; i++) {
        pids[i] = fork();

        if (pids[i] == -1) {
            perror("fork failed");
            break;
        }

        if (pids[i] == 0) {
            /* Connect input pipe */
            if (i > 0) {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1) {
                    perror("dup2 input pipe");
                    _exit(EXIT_FAILURE);
                }
            }

            /* Connect output pipe */
            if (i < command_count - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    perror("dup2 output pipe");
                    _exit(EXIT_FAILURE);
                }
            }

            /* Close pipe descriptors */
            for (int j = 0; j < command_count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            /* Input redirection */
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

            /* Output redirection */
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
            _exit(127);
        }

        launched++;
    }

    /* Close pipes in parent */
    for (int i = 0; i < command_count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    /* Background execution */
    if (background) {
        for (int i = 0; i < launched; i++)
            add_background_job(pids[i], command_text);

        return launched == command_count ? 0 : 1;
    }

    /* Foreground execution and status collection */
    int result_status = 0;

    for (int i = 0; i < launched; i++) {
        int status;
        pid_t result;

        do {
            result = waitpid(pids[i], &status, 0);
        } while (result == -1 && errno == EINTR);

        if (result == -1) {
            perror("waitpid failed");
            result_status = 1;
            continue;
        }

        /* Use the exit status of the final pipeline command */
        if (i == launched - 1) {
            if (WIFEXITED(status))
                result_status = WEXITSTATUS(status);
            else if (WIFSIGNALED(status))
                result_status = 128 + WTERMSIG(status);
            else
                result_status = 1;
        }
    }

    if (launched != command_count)
        return 1;

    return result_status;
}

/* Execute commands separated by ; and & */
int execute(char** arglist) {
    if (arglist == NULL || arglist[0] == NULL)
        return 0;

    check_background_jobs();

    int start = 0;

    for (int i = 0; ; i++) {
        if (arglist[i] == NULL ||
            strcmp(arglist[i], ";") == 0 ||
            strcmp(arglist[i], "&") == 0) {

            int end = i;
            int background = 0;

            if (arglist[i] != NULL &&
                strcmp(arglist[i], "&") == 0)
                background = 1;

            if (end > start) {
                char* segment[MAXARGS + 1];
                char command_text[MAX_LEN];

                int count = end - start;

                if (count > MAXARGS) {
                    fprintf(stderr, "Too many command arguments\n");
                    last_command_status = 1;
                    return 1;
                }

                for (int j = 0; j < count; j++)
                    segment[j] = arglist[start + j];

                segment[count] = NULL;

                /* Preserve the original command text for job reporting */
                command_text[0] = '\0';

                for (int j = 0; j < count; j++) {
                    if (j > 0)
                        strncat(command_text, " ",
                                sizeof(command_text) -
                                strlen(command_text) - 1);

                    strncat(command_text, segment[j],
                            sizeof(command_text) -
                            strlen(command_text) - 1);
                }

                /* Expand variables for this command segment */
                expand_variables(segment);

                /*
                 * Expansion replaces allocated argument strings.
                 * Copy updated pointers back to the original list
                 * so main.c can safely free them.
                 */
                for (int j = 0; j < count; j++)
                    arglist[start + j] = segment[j];

                last_command_status =
                    execute_pipeline(segment, background, command_text);
            }

            if (arglist[i] == NULL)
                break;

            start = i + 1;
        }
    }

    return last_command_status;
}

/* Execute a condition and return its exit status */
int execute_status(char** arglist) {
    execute(arglist);
    return last_command_status;
}
