#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <setjmp.h>

#define MAX_COMMAND_SZ 256

typedef struct History {
    pid_t pid;
    char command[100];
    int background;
} History;


void handleCommand(char command[MAX_COMMAND_SZ]);

char **str_split(char *a_str, const char a_delim);

void freeTokens(char **pString);

void addHistory(char command[256], __pid_t pid, int background);

void print(int historyOrJobs);

void printHistory();

void printJobs();

History history[100];
int historyCount = 0;

char *concat(const char *s1, const char *s2) {
    char *result = malloc(strlen(s1) + strlen(s2) + 1); // +1 for the null-terminator
    // in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

int main() {
    char command[MAX_COMMAND_SZ];
    while (1) {
        printf("> ");
        fgets(command, MAX_COMMAND_SZ, stdin);
        strtok(command, "\n");
        if (strcmp(command, "exit") == 0) {
            printf("%d", getpid());
            exit(EXIT_SUCCESS);
        } else if (strcmp(command, "history") == 0) {
            addHistory(command, getpid(), 0);
            print(1);
            // printHistory();
        } else if (strcmp(command, "jobs") == 0) {
            addHistory(command, getpid(), 0);
            print(0);
            // printJobs();
        } else if (command[0] == 'c' && command[1] == 'd' && command[2] == ' ') {
            addHistory(command, getpid(), 0);
            printf("%d", getpid());
            printf("handleCD\n");
        } else {
            handleCommand(command);
        }
    }
}

void printJobs() {
    int i = 0, status, running = 0;
    for (i = 0; i < historyCount; i++) {
        char statusString[10] = "DONE";
        if ((waitpid(history[i].pid, &status, WNOHANG) == 0)
            // ||  (i == historyCount - 1 && strcmp(history[i].command, "history") == 0)
                ) {
            printf("wierd\n");
            strcpy(statusString, "RUNNING");
            running = 1;
        }
        if (running) {
            strcpy(statusString, "");
            printf("%d %s %s\n", history[i].pid, history[i].command, statusString);
            running = 0;
        }
    }
}

void printHistory() {
    int i = 0, status, running = 0;
    for (i = 0; i < historyCount; i++) {
        char statusString[10] = "DONE";
        if ((waitpid(history[i].pid, &status, WNOHANG) == 0)
            || (i == historyCount - 1 && strcmp(history[i].command, "history") == 0)
                ) {
            strcpy(statusString, "RUNNING");
        }
        printf("%d %s %s\n", history[i].pid, history[i].command, statusString);
    }
}

// historyOrJobs - 1 for history, 0 for jobs
void print(int historyOrJobs) {
    int i = 0, status, running = 0;
    for (i = 0; i < historyCount; i++) {
        char statusString[10] = "DONE";
        if ((waitpid(history[i].pid, &status, WNOHANG) == 0)
            || (i == historyCount - 1 && strcmp(history[i].command, "history") == 0)
                ) {
            strcpy(statusString, "RUNNING");
            running = 1;
        }
        if (historyOrJobs) {
            printf("%d %s %s\n", history[i].pid, history[i].command, statusString);
        } else if (running) {
            strcpy(statusString, "");
            printf("%d %s %s\n", history[i].pid, history[i].command, statusString);
            running = 0;
        }
    }
}

void addHistory(char command[256], __pid_t pid, int background) {
    history[historyCount].pid = pid;
    history[historyCount].background = background;
    strcpy(history[historyCount].command, command);
    historyCount++;
}

void handleCommand(char command[MAX_COMMAND_SZ]) {
    pid_t pid;
    int background = 0;
    int len = strlen(command);
    if (command[len - 1] == '&' && command[len - 2] == ' ') {
        background = 1;
    }

    if ((pid = fork()) < 0) {
        printf("fork error\n");
    } else if (pid == 0) {
        if (background) {
            while (1) {
                sleep(10);
                exit(0);
            }
        } else { // foreground
            // spliting words of the command to array of strings
            char **commandToExec = str_split(command, ' ');

            /*
             *             if (commandToExec) { // print the splitted  command
                int i;
                for (i = 0; *(commandToExec + i); i++) {
                    printf("command[%d] = %s | ", i, *(commandToExec + i));
                }
                printf("\n");
            }
             * */
            if (execvp(commandToExec[0], commandToExec) == -1){
                printf("error in execvp\n");
            }
            freeTokens(commandToExec);
            exit(0);
        }
    } else { // pid > 0
        addHistory(command, pid, background);
        printf("%d\n", pid);
        if (!background) { // don't wait for background to finish
            if (waitpid(pid, NULL, 0) != pid) {
                printf("waitpid error\n");
            }
        }
    }
}

char **str_split(char *a_str, const char a_delim) {
    char **result = 0;
    size_t count = 0;
    char *tmp = a_str;
    char *last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    while (*tmp) {
        if (a_delim == *tmp) {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    count += last_comma < (a_str + strlen(a_str) - 1);

    count++;

    result = malloc(sizeof(char *) * count);

    if (result) {
        size_t idx = 0;
        char *token = strtok(a_str, delim);

        while (token) {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

void freeTokens(char **pString) {
    int i;
    for (i = 0; *(pString + i); i++) {
        free(*(pString + i));
    }
    free(pString);
}
