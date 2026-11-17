
#include "shell.h"

/*
 * Check whether a command is a shell built-in.
 */
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

/*
 * Display available shell commands.
 */
void display_help(void) {
    printf("\n");
    printf("========== FCIT CUSTOM SHELL ==========\n");
    printf("Built-in commands:\n");
    printf("  cd [directory]  Change directory\n");
    printf("  exit            Exit the shell\n");
    printf("  help            Display this help message\n");
    printf("  jobs            Display background jobs\n");
    printf("\nExternal commands:\n");
    printf("  Any executable available through PATH\n");
    printf("=======================================\n\n");
}

/*
 * Display background jobs.
 * Full job tracking will be implemented with multitasking.
 */
void display_jobs(void) {
    printf("No background jobs to display.\n");
}

/*
 * Execute shell built-in commands in the parent process.
 */
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

    return 0;
}

/*
 * Execute a command.
 */
int execute(char** arglist) {

    int status;
    pid_t cpid;

    /* Handle built-in commands first */
    if (is_builtin(arglist[0])) {
        execute_builtin(arglist);
        return 0;
    }

    /* Execute external commands */
    cpid = fork();

    if (cpid == -1) {
        perror("fork failed");
        return -1;
    }

    if (cpid == 0) {
        execvp(arglist[0], arglist);
        perror("Command not found");
        exit(EXIT_FAILURE);
    }

    if (waitpid(cpid, &status, 0) == -1) {
        perror("waitpid failed");
        return -1;
    }

    return 0;
}

