#include "commands.h"

extern char **environ;

/* Helper for standard error reporting */
static void perr(const char *cmd, const char *arg) {
    fprintf(stderr, BOLD FG_BR_RED "  ✖  " RESET FG_RED "%s: %s: %s\n" RESET, cmd, arg, strerror(errno));
}

/* Helper to convert mode to string for ls -l */
static void mode_to_str(mode_t mode, char *str) {
    strcpy(str, "----------");
    if (S_ISDIR(mode))  str[0] = 'd';
    if (S_ISLNK(mode))  str[0] = 'l';
    if (S_ISFIFO(mode)) str[0] = 'p';
    if (S_ISSOCK(mode)) str[0] = 's';
    if (S_ISCHR(mode))  str[0] = 'c';
    if (S_ISBLK(mode))  str[0] = 'b';

    if (mode & S_IRUSR) str[1] = 'r';
    if (mode & S_IWUSR) str[2] = 'w';
    if (mode & S_IXUSR) str[3] = 'x';
    if (mode & S_IRGRP) str[4] = 'r';
    if (mode & S_IWGRP) str[5] = 'w';
    if (mode & S_IXGRP) str[6] = 'x';
    if (mode & S_IROTH) str[7] = 'r';
    if (mode & S_IWOTH) str[8] = 'w';
    if (mode & S_IXOTH) str[9] = 'x';
}

/* 1. ls — IMPROVED: Added -l (long format) and -a support */
static int cmd_ls(char **args, int argc) {
    int hidden = 0, long_fmt = 0;
    const char *dir_path = ".";

    for (int i = 1; i < argc; i++) {
        if (args[i][0] == '-') {
            if (strchr(args[i], 'a')) hidden = 1;
            if (strchr(args[i], 'l')) long_fmt = 1;
        } else {
            dir_path = args[i];
        }
    }

    DIR *d = opendir(dir_path);
    if (!d) {
        perr("ls", dir_path);
        return 1;
    }

    struct dirent *entry;
    while ((entry = readdir(d))) {
        if (!hidden && entry->d_name[0] == '.') continue;

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        struct stat st;
        lstat(full_path, &st);

        if (long_fmt) {
            char mstr[11];
            mode_to_str(st.st_mode, mstr);
            struct passwd *pw = getpwuid(st.st_uid);
            struct group  *gr = getgrgid(st.st_gid);
            char tstr[32];
            strftime(tstr, sizeof(tstr), "%b %d %H:%M", localtime(&st.st_mtime));

            printf("%s %2ld %-8s %-8s %8lld %s ",
                   mstr, (long)st.st_nlink,
                   pw ? pw->pw_name : "unknown",
                   gr ? gr->gr_name : "unknown",
                   (long long)st.st_size, tstr);
        }

        if (S_ISDIR(st.st_mode))      printf(BOLD FG_BR_BLUE  "%s" RESET, entry->d_name);
        else if (S_ISLNK(st.st_mode)) printf(BOLD FG_BR_CYAN  "%s" RESET, entry->d_name);
        else if (st.st_mode & S_IXUSR) printf(BOLD FG_BR_GREEN "%s" RESET, entry->d_name);
        else                           printf("%s", entry->d_name);
        
        printf(long_fmt ? "\n" : "  ");
    }
    if (!long_fmt) printf("\n");

    closedir(d);
    return 0;
}

/* 2. mkdir — IMPROVED: Expanded for readability */
static int cmd_mkdir(char **args, int argc) {
    if (argc < 2) {
        fprintf(stderr, FG_BR_RED "Usage: mkdir <directory>\n" RESET);
        return 1;
    }
    int status = 0;
    for (int i = 1; i < argc; i++) {
        if (mkdir(args[i], 0755) != 0) {
            perr("mkdir", args[i]);
            status = 1;
        } else {
            printf(FG_BR_GREEN "  Created directory: " RESET "%s\n", args[i]);
        }
    }
    return status;
}

/* 3. rmdir — IMPROVED: Expanded for readability */
static int cmd_rmdir(char **args, int argc) {
    if (argc < 2) {
        fprintf(stderr, FG_BR_RED "Usage: rmdir <directory>\n" RESET);
        return 1;
    }
    int status = 0;
    for (int i = 1; i < argc; i++) {
        if (rmdir(args[i]) != 0) {
            perr("rmdir", args[i]);
            status = 1;
        } else {
            printf(FG_BR_GREEN "  Removed directory: " RESET "%s\n", args[i]);
        }
    }
    return status;
}

/* 4. touch — IMPROVED: Expanded for readability */
static int cmd_touch(char **args, int argc) {
    if (argc < 2) {
        fprintf(stderr, FG_BR_RED "Usage: touch <file>\n" RESET);
        return 1;
    }
    int status = 0;
    for (int i = 1; i < argc; i++) {
        int fd = open(args[i], O_WRONLY | O_CREAT, 0644);
        if (fd < 0) {
            perr("touch", args[i]);
            status = 1;
        } else {
            close(fd);
            printf(FG_BR_GREEN "  Touched file: " RESET "%s\n", args[i]);
        }
    }
    return status;
}

/* 5. rm — IMPROVED: Expanded for readability */
static int recursive_rm(const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0) {
        perr("rm", path);
        return 1;
    }

    if (S_ISDIR(st.st_mode)) {
        DIR *d = opendir(path);
        if (!d) {
            perr("rm", path);
            return 1;
        }
        struct dirent *entry;
        int status = 0;
        while ((entry = readdir(d))) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
            char sub_path[PATH_MAX];
            snprintf(sub_path, sizeof(sub_path), "%s/%s", path, entry->d_name);
            status |= recursive_rm(sub_path);
        }
        closedir(d);
        if (rmdir(path) != 0) {
            perr("rm", path);
            return 1;
        }
        return status;
    }

    if (unlink(path) != 0) {
        perr("rm", path);
        return 1;
    }
    return 0;
}

static int cmd_rm(char **args, int argc) {
    if (argc < 2) {
        fprintf(stderr, FG_BR_RED "Usage: rm [-r] <path>\n" RESET);
        return 1;
    }
    int recursive = 0, start_idx = 1;
    if (strcmp(args[1], "-r") == 0 || strcmp(args[1], "-rf") == 0) {
        recursive = 1;
        start_idx = 2;
    }

    int status = 0;
    for (int i = start_idx; i < argc; i++) {
        if (recursive) {
            status |= recursive_rm(args[i]);
        } else {
            if (unlink(args[i]) != 0) {
                perr("rm", args[i]);
                status = 1;
            } else {
                printf(FG_BR_GREEN "  Deleted: " RESET "%s\n", args[i]);
            }
        }
    }
    return status;
}

/* 6. cp — IMPROVED: Expanded for readability */
static int cmd_cp(char **args, int argc) {
    if (argc < 3) {
        fprintf(stderr, FG_BR_RED "Usage: cp <source> <destination>\n" RESET);
        return 1;
    }
    int src_fd = open(args[1], O_RDONLY);
    if (src_fd < 0) {
        perr("cp", args[1]);
        return 1;
    }
    int dst_fd = open(args[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst_fd < 0) {
        perr("cp", args[2]);
        close(src_fd);
        return 1;
    }

    char buffer[8192];
    ssize_t bytes_read;
    while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
        write(dst_fd, buffer, bytes_read);
    }

    close(src_fd);
    close(dst_fd);
    printf(FG_BR_GREEN "  Copied: " RESET "%s -> %s\n", args[1], args[2]);
    return 0;
}

/* 7. mv — IMPROVED: Expanded for readability */
static int cmd_mv(char **args, int argc) {
    if (argc < 3) {
        fprintf(stderr, FG_BR_RED "Usage: mv <source> <destination>\n" RESET);
        return 1;
    }
    if (rename(args[1], args[2]) == 0) {
        printf(FG_BR_GREEN "  Moved: " RESET "%s -> %s\n", args[1], args[2]);
        return 0;
    }
    
    // Cross-device move fallback
    if (cmd_cp(args, argc) == 0) {
        if (unlink(args[1]) == 0) {
            return 0;
        }
        perr("mv (delete)", args[1]);
    }
    return 1;
}

/* 8. cat — IMPROVED: Expanded for readability */
static int cmd_cat(char **args, int argc) {
    if (argc < 2) {
        fprintf(stderr, FG_BR_RED "Usage: cat <file>\n" RESET);
        return 1;
    }
    int status = 0;
    for (int i = 1; i < argc; i++) {
        FILE *f = fopen(args[i], "r");
        if (!f) {
            perr("cat", args[i]);
            status = 1;
            continue;
        }
        char line[4096];
        while (fgets(line, sizeof(line), f)) {
            printf("%s", line);
        }
        fclose(f);
    }
    return status;
}

/* 9. head — IMPROVED: Expanded for readability */
static int cmd_head(char **args, int argc) {
    int num_lines = 10, file_idx = 1;
    if (argc >= 3 && strcmp(args[1], "-n") == 0) {
        num_lines = atoi(args[2]);
        file_idx = 3;
    }
    if (file_idx >= argc) {
        fprintf(stderr, FG_BR_RED "Usage: head [-n lines] <file>\n" RESET);
        return 1;
    }
    FILE *f = fopen(args[file_idx], "r");
    if (!f) {
        perr("head", args[file_idx]);
        return 1;
    }
    char buffer[4096];
    int count = 0;
    while (count < num_lines && fgets(buffer, sizeof(buffer), f)) {
        printf("%s", buffer);
        count++;
    }
    fclose(f);
    return 0;
}

/* 10. tail — IMPROVED: Expanded for readability */
static int cmd_tail(char **args, int argc) {
    int num_lines = 10, file_idx = 1;
    if (argc >= 3 && strcmp(args[1], "-n") == 0) {
        num_lines = atoi(args[2]);
        file_idx = 3;
    }
    if (file_idx >= argc) {
        fprintf(stderr, FG_BR_RED "Usage: tail [-n lines] <file>\n" RESET);
        return 1;
    }
    FILE *f = fopen(args[file_idx], "r");
    if (!f) {
        perr("tail", args[file_idx]);
        return 1;
    }
    
    char **ring = calloc(num_lines, sizeof(char *));
    char buffer[4096];
    int line_count = 0;
    while (fgets(buffer, sizeof(buffer), f)) {
        int idx = line_count % num_lines;
        if (ring[idx]) free(ring[idx]);
        ring[idx] = strdup(buffer);
        line_count++;
    }
    fclose(f);

    int start = (line_count > num_lines) ? (line_count % num_lines) : 0;
    int limit = (line_count > num_lines) ? num_lines : line_count;
    for (int i = 0; i < limit; i++) {
        int idx = (start + i) % num_lines;
        printf("%s", ring[idx]);
        free(ring[idx]);
    }
    free(ring);
    return 0;
}

/* 11. wc — IMPROVED: Expanded for readability */
static int cmd_wc(char **args, int argc) {
    if (argc < 2) {
        fprintf(stderr, FG_BR_RED "Usage: wc <file>\n" RESET);
        return 1;
    }
    FILE *f = fopen(args[1], "r");
    if (!f) {
        perr("wc", args[1]);
        return 1;
    }
    long lines = 0, words = 0, chars = 0;
    int ch, in_word = 0;
    while ((ch = fgetc(f)) != EOF) {
        chars++;
        if (ch == '\n') lines++;
        if (isspace(ch)) {
            in_word = 0;
        } else if (!in_word) {
            words++;
            in_word = 1;
        }
    }
    fclose(f);
    printf(FG_BR_YELLOW "  %8ld lines  %8ld words  %8ld chars  " RESET "%s\n", 
           lines, words, chars, args[1]);
    return 0;
}

/* 12. chmod — IMPROVED: Expanded for readability */
static int cmd_chmod(char **args, int argc) {
    if (argc < 3) {
        fprintf(stderr, FG_BR_RED "Usage: chmod <mode_octal> <file>\n" RESET);
        return 1;
    }
    mode_t mode = (mode_t)strtol(args[1], NULL, 8);
    int status = 0;
    for (int i = 2; i < argc; i++) {
        if (chmod(args[i], mode) != 0) {
            perr("chmod", args[i]);
            status = 1;
        } else {
            printf(FG_BR_GREEN "  Changed mode: " RESET "%s\n", args[i]);
        }
    }
    return status;
}

/* 13. date — IMPROVED: Expanded for readability */
static int cmd_date(char **args, int argc) {
    (void)args; (void)argc;
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char buffer[128];
    strftime(buffer, sizeof(buffer), "%A, %d %B %Y - %H:%M:%S %Z", tm_info);
    printf(BOLD FG_BR_CYAN "  Current Date: " RESET "%s\n", buffer);
    return 0;
}

/* 14. whoami — IMPROVED: Expanded for readability */
static int cmd_whoami(char **args, int argc) {
    (void)args; (void)argc;
    struct passwd *pw = getpwuid(getuid());
    if (pw) {
        printf(FG_BR_GREEN "%s\n" RESET, pw->pw_name);
    } else {
        printf(FG_BR_RED "Unknown User\n" RESET);
    }
    return 0;
}

/* 15. uname — IMPROVED: Expanded for readability */
static int cmd_uname(char **args, int argc) {
    struct utsname info;
    uname(&info);
    int all = 0;
    if (argc > 1 && strcmp(args[1], "-a") == 0) all = 1;
    
    if (all) {
        printf(FG_BR_CYAN "%s %s %s %s %s\n" RESET, 
               info.sysname, info.nodename, info.release, info.version, info.machine);
    } else {
        printf(FG_BR_CYAN "%s\n" RESET, info.sysname);
    }
    return 0;
}

/* 16. env — IMPROVED: Expanded for readability */
static int cmd_env(char **args, int argc) {
    (void)args; (void)argc;
    for (char **e = environ; *e != NULL; e++) {
        printf("%s\n", *e);
    }
    return 0;
}

/* 17. export — IMPROVED: Expanded for readability */
static int cmd_export(char **args, int argc) {
    if (argc < 2) {
        fprintf(stderr, FG_BR_RED "Usage: export VAR=VALUE\n" RESET);
        return 1;
    }
    for (int i = 1; i < argc; i++) {
        char *equal = strchr(args[i], '=');
        if (!equal) {
            fprintf(stderr, FG_BR_RED "export: invalid syntax: %s\n" RESET, args[i]);
            continue;
        }
        *equal = '\0';
        if (setenv(args[i], equal + 1, 1) != 0) {
            perr("export", args[i]);
        } else {
            printf(FG_BR_GREEN "  Set environment: " RESET "%s=%s\n", args[i], equal + 1);
        }
        *equal = '=';
    }
    return 0;
}

/* 18. unset — IMPROVED: Expanded for readability */
static int cmd_unset(char **args, int argc) {
    if (argc < 2) {
        fprintf(stderr, FG_BR_RED "Usage: unset VAR\n" RESET);
        return 1;
    }
    for (int i = 1; i < argc; i++) {
        if (unsetenv(args[i]) != 0) {
            perr("unset", args[i]);
        } else {
            printf(FG_BR_GREEN "  Unset variable: " RESET "%s\n", args[i]);
        }
    }
    return 0;
}

/* 19. type — IMPROVED: Expanded for readability */
static int cmd_type(char **args, int argc) {
    if (argc < 2) {
        fprintf(stderr, FG_BR_RED "Usage: type <command>\n" RESET);
        return 1;
    }
    static const char *builtins[] = {
        "cd", "pwd", "echo", "clear", "history", "help", "about", "exit",
        "ls", "mkdir", "rmdir", "touch", "rm", "cp", "mv", "cat", "head", 
        "tail", "wc", "chmod", "date", "whoami", "uname", "env", "export", 
        "unset", "type", "sleep", "kill", "stat", "ln", "basename", 
        "dirname", "realpath", "which", "printenv", "true", "false", NULL
    };

    for (int i = 1; i < argc; i++) {
        int found = 0;
        for (int j = 0; builtins[j]; j++) {
            if (strcmp(args[i], builtins[j]) == 0) {
                printf(FG_BR_CYAN "%s" RESET " is a shell builtin\n", args[i]);
                found = 1;
                break;
            }
        }
        if (!found) {
            char *path = getenv("PATH");
            if (path) {
                char path_copy[8192];
                strncpy(path_copy, path, sizeof(path_copy) - 1);
                char *dir = strtok(path_copy, ":");
                while (dir) {
                    char full[PATH_MAX];
                    snprintf(full, sizeof(full), "%s/%s", dir, args[i]);
                    if (access(full, X_OK) == 0) {
                        printf(FG_BR_GREEN "%s" RESET " is %s\n", args[i], full);
                        found = 1;
                        break;
                    }
                    dir = strtok(NULL, ":");
                }
            }
        }
        if (!found) {
            fprintf(stderr, FG_BR_RED "type: %s: not found\n" RESET, args[i]);
        }
    }
    return 0;
}

/* 20. sleep — IMPROVED: Expanded for readability */
static int cmd_sleep(char **args, int argc) {
    if (argc < 2) {
        fprintf(stderr, FG_BR_RED "Usage: sleep <seconds>\n" RESET);
        return 1;
    }
    unsigned sec = (unsigned)atoi(args[1]);
    printf(DIM "  Sleeping for %u seconds..." RESET, sec);
    fflush(stdout);
    sleep(sec);
    printf("\r\033[K");
    return 0;
}

/* 21. kill — IMPROVED: Expanded for readability */
static int cmd_kill(char **args, int argc) {
    if (argc < 2) {
        fprintf(stderr, FG_BR_RED "Usage: kill [-signal] <pid>\n" RESET);
        return 1;
    }
    int sig = SIGTERM, start = 1;
    if (args[1][0] == '-') {
        sig = atoi(args[1] + 1);
        start = 2;
    }
    for (int i = start; i < argc; i++) {
        pid_t pid = (pid_t)atoi(args[i]);
        if (kill(pid, sig) != 0) {
            perr("kill", args[i]);
        } else {
            printf(FG_BR_GREEN "  Sent signal %d to PID %d\n" RESET, sig, (int)pid);
        }
    }
    return 0;
}

/* 22. stat — IMPROVED: Expanded for readability */
static int cmd_stat(char **args, int argc) {
    if (argc < 2) {
        fprintf(stderr, FG_BR_RED "Usage: stat <file>\n" RESET);
        return 1;
    }
    for (int i = 1; i < argc; i++) {
        struct stat st;
        if (lstat(args[i], &st) != 0) {
            perr("stat", args[i]);
            continue;
        }
        printf(BOLD FG_BR_CYAN "  File: " RESET "%s\n", args[i]);
        printf(BOLD FG_BR_YELLOW "  Size: " RESET "%lld bytes\n", (long long)st.st_size);
        printf(BOLD FG_BR_YELLOW "  Inode:" RESET "%llu\n", (unsigned long long)st.st_ino);
        printf(BOLD FG_BR_YELLOW "  Links:" RESET "%u\n", (unsigned int)st.st_nlink);
        char tbuf[64];
        strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", localtime(&st.st_mtime));
        printf(BOLD FG_BR_YELLOW "  Last Modified: " RESET "%s\n\n", tbuf);
    }
    return 0;
}

/* 23. ln — IMPROVED: Expanded for readability */
static int cmd_ln(char **args, int argc) {
    if (argc < 3) {
        fprintf(stderr, FG_BR_RED "Usage: ln [-s] <target> <link>\n" RESET);
        return 1;
    }
    int sym = 0, t_idx = 1, l_idx = 2;
    if (strcmp(args[1], "-s") == 0) {
        sym = 1; t_idx = 2; l_idx = 3;
    }
    if (l_idx >= argc) return 1;
    int res = sym ? symlink(args[t_idx], args[l_idx]) : link(args[t_idx], args[l_idx]);
    if (res != 0) {
        perr("ln", args[l_idx]);
        return 1;
    }
    printf(FG_BR_GREEN "  Linked: " RESET "%s -> %s\n", args[l_idx], args[t_idx]);
    return 0;
}

/* 24. basename — IMPROVED: Expanded for readability */
static int cmd_basename(char **args, int argc) {
    if (argc < 2) return 1;
    char *base = strrchr(args[1], '/');
    printf("%s\n", base ? base + 1 : args[1]);
    return 0;
}

/* 25. dirname — IMPROVED: Expanded for readability */
static int cmd_dirname(char **args, int argc) {
    if (argc < 2) return 1;
    char *last = strrchr(args[1], '/');
    if (!last) {
        printf(".\n");
    } else if (last == args[1]) {
        printf("/\n");
    } else {
        *last = '\0';
        printf("%s\n", args[1]);
    }
    return 0;
}

/* 26. realpath — IMPROVED: Expanded for readability */
static int cmd_realpath(char **args, int argc) {
    if (argc < 2) return 1;
    char buf[PATH_MAX];
    if (realpath(args[1], buf)) {
        printf("%s\n", buf);
        return 0;
    }
    perr("realpath", args[1]);
    return 1;
}

/* 27. which — IMPROVED: Expanded for readability */
static int cmd_which(char **args, int argc) {
    if (argc < 2) return 1;
    char *path = getenv("PATH");
    if (!path) return 1;
    char path_copy[8192];
    strncpy(path_copy, path, sizeof(path_copy) - 1);
    char *dir = strtok(path_copy, ":");
    while (dir) {
        char full[PATH_MAX];
        snprintf(full, sizeof(full), "%s/%s", dir, args[1]);
        if (access(full, X_OK) == 0) {
            printf("%s\n", full);
            return 0;
        }
        dir = strtok(NULL, ":");
    }
    return 1;
}

/* 28. printenv — IMPROVED: Expanded for readability */
static int cmd_printenv(char **args, int argc) {
    if (argc < 2) {
        return cmd_env(args, argc);
    }
    char *val = getenv(args[1]);
    if (val) printf("%s\n", val);
    return val ? 0 : 1;
}

/* 29. true — IMPROVED: Expanded for readability */
static int cmd_true(char **args, int argc) {
    (void)args; (void)argc;
    return 0;
}

/* 30. false — IMPROVED: Expanded for readability */
static int cmd_false(char **args, int argc) {
    (void)args; (void)argc;
    return 1;
}

/* Dispatch Table */
typedef struct {
    const char *name;
    int (*func)(char **, int);
} Command;

static const Command COMMAND_TABLE[] = {
    {"ls", cmd_ls}, {"mkdir", cmd_mkdir}, {"rmdir", cmd_rmdir}, {"touch", cmd_touch},
    {"rm", cmd_rm}, {"cp", cmd_cp}, {"mv", cmd_mv}, {"cat", cmd_cat},
    {"head", cmd_head}, {"tail", cmd_tail}, {"wc", cmd_wc}, {"chmod", cmd_chmod},
    {"date", cmd_date}, {"whoami", cmd_whoami}, {"uname", cmd_uname}, {"env", cmd_env},
    {"export", cmd_export}, {"unset", cmd_unset}, {"type", cmd_type}, {"sleep", cmd_sleep},
    {"kill", cmd_kill}, {"stat", cmd_stat}, {"ln", cmd_ln}, {"basename", cmd_basename},
    {"dirname", cmd_dirname}, {"realpath", cmd_realpath}, {"which", cmd_which},
    {"printenv", cmd_printenv}, {"true", cmd_true}, {"false", cmd_false},
    {NULL, NULL}
};

int is_ext_cmd(const char *name) {
    for (int i = 0; COMMAND_TABLE[i].name; i++) {
        if (strcmp(name, COMMAND_TABLE[i].name) == 0) return 1;
    }
    return 0;
}

int run_ext_cmd(char **args, int argc) {
    for (int i = 0; COMMAND_TABLE[i].name; i++) {
        if (strcmp(args[0], COMMAND_TABLE[i].name) == 0) {
            return COMMAND_TABLE[i].func(args, argc);
        }
    }
    return -1;
}
