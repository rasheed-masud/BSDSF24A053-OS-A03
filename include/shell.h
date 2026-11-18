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

#define MAX_LEN 512
#define MAXARGS 50
#define ARGLEN 100
#define PROMPT "FCIT> "

char* read_cmd(char* prompt, FILE* fp);
char** tokenize(char* cmdline);
int execute(char** arglist);

int is_builtin(char* command);
int execute_builtin(char** arglist);
void display_help(void);
void display_jobs(void);

#define HISTORY_SIZE 20

void save_command_history(const char* command);
void print_history(void);
const char* get_history(int command_number);

#endif
