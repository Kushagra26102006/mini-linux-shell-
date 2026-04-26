/**
 * commands.h — 30+ built-in command declarations for Mini Linux Shell
 */
#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <time.h>
#include <limits.h>
#include <signal.h>

/* ANSI palette */
#ifndef RESET
#define RESET           "\033[0m"
#define BOLD            "\033[1m"
#define DIM             "\033[2m"
#define FG_RED          "\033[31m"
#define FG_GREEN        "\033[32m"
#define FG_YELLOW       "\033[33m"
#define FG_BLUE         "\033[34m"
#define FG_MAGENTA      "\033[35m"
#define FG_CYAN         "\033[36m"
#define FG_WHITE        "\033[37m"
#define FG_BR_RED       "\033[91m"
#define FG_BR_GREEN     "\033[92m"
#define FG_BR_YELLOW    "\033[93m"
#define FG_BR_BLUE      "\033[94m"
#define FG_BR_MAGENTA   "\033[95m"
#define FG_BR_CYAN      "\033[96m"
#define FG_BR_WHITE     "\033[97m"
#endif

/* Public API */
int is_ext_cmd(const char *name);
int run_ext_cmd(char **args, int argc);

#endif /* COMMANDS_H */
