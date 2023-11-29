#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>

int MAX_LENGTH = 100;
int MAX_COMMANDS = 100;


void display_prompt() {
    static int first_time = 1;
    if (first_time) {
        const char *CLEAR_SCREEN_ANSI = " \e[1;1H\e[2J";
        write(STDOUT_FILENO, CLEAR_SCREEN_ANSI, 12);
        first_time = 0;
    }
    printf("$  ");
}

void read_command(char cmd[], char *par[]) {
    int count = 0, i = 0, j;
    char *array[100], *p;
    char line[1024];
    for (;;) {
        int c = fgetc(stdin);
        line[count++] = (char) c;
        if (c == '\n') {
            break;
        }
    }
    if (count == 1) {
        return;
    }
    p = strtok(line, " \n");
    // parse the line into words
    while (p != NULL) {
        array[i++] = strdup(p);
        p = strtok(NULL, "\n");
    }
    // first word is the command
    strcpy(cmd, array[0]);
    // all the rest are parameters
    for (j = 0; j < i; j++) {
        par[j] = array[j];
    }
    par[i] = NULL;
}

struct p_info {
    int p_id;
    char command[100];
};

void history(struct p_info p[], int cout) {
    for (int i = 0; i < cout; i++) {
        printf("%d", p[i].p_id);
        printf(" %s\n", p[i].command);
    }
}

int cd(char folder_name[]) {
    DIR *pDir = opendir (folder_name);
    if (pDir == NULL) {
        printf ("Cannot open directory '%s'\n", folder_name);
        return 1;
    }
}


int main(int argc, char* argv[]) {
    char cmd[100], command[100], *parameters[20];
    struct p_info history_commands[MAX_COMMANDS];
    int cout = 0, pid;
    char *envp[] = {(char *) "PATH=/bin", 0};
   char *path = getenv("PATH");
   int i;
   for (i = 1; i < argc; i++) {
       strcat(path, ":");
       strcat(path, argv[i]);
   }
   if (setenv("PATH", path, 1) != 0) {
       perror("setenv failed");
       exit(1);
   }
    while (1) {
        display_prompt();
        read_command(command, parameters);
        if (strcmp(command, "exit") == 0) {
            exit(1);
        } else if (strcmp(command, "cd") == 0) {
            history_commands[cout].p_id = getpid();
            strncpy(history_commands[cout].command, command, MAX_LENGTH);
            cout++;
            cd(parameters[1]);
        } else if (strcmp(command, "history") == 0) {
            history_commands[cout].p_id = getpid();
            strncpy(history_commands[cout].command, command, MAX_LENGTH);
            cout++;
            history(history_commands, cout);
        } else {
            pid = fork();

            if (pid != 0) {
                history_commands[cout].p_id = pid;
                strncpy(history_commands[cout].command, command, MAX_LENGTH);
                cout++;
                wait(NULL);
            } else {
                //strcpy(cmd, "/bin/");
                //strcat(cmd, command);
                if (execvp(command, parameters) == -1) {
                  perror("failed");
                }
            }
        }
    }
}}
