
#ifndef SHELL_H
#define SHELL_H

#include <readline/readline.h>
#include <readline/history.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_LEN 512
#define MAXARGS 50
#define ARGLEN 100
#define PROMPT "FCIT> "

#define HISTORY_SIZE 20
#define MAX_JOBS 100

typedef struct {
    pid_t pid;
    int job_id;
    char command[MAX_LEN];
    int active;
} Job;

/* Shell variable linked list */
typedef struct ShellVariable {
    char* name;
    char* value;
    struct ShellVariable* next;
} ShellVariable;

char* read_cmd(char* prompt, FILE* fp);
char** tokenize(char* cmdline);

int execute(char** arglist);

/* Execute a command and return its actual exit status */
int execute_status(char** arglist);

int is_builtin(char* command);
int execute_builtin(char** arglist);
void display_help(void);
void display_jobs(void);

/* Shell variable management */
int is_variable_assignment(const char* token);
int assign_variable(const char* assignment);
void expand_variables(char** arglist);
void print_variables(void);
void free_variables(void);

void save_command_history(const char* command);
void print_history(void);
const char* get_history(int command_number);

void add_background_job(pid_t pid, const char* command);
void check_background_jobs(void);
void print_background_jobs(void);

#endif
