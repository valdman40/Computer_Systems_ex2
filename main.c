#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#define MAX_COMMAND_SZ 256

void handleCommand(char command[MAX_COMMAND_SZ]);

char **str_split(char *a_str, const char a_delim);

void freeTokens(char **pString);

char *history[100];
int historyCount = 0;

int main() {
    char command[MAX_COMMAND_SZ];
    int status;
    while (1) {
        printf("\n>");
        fgets(command, MAX_COMMAND_SZ, stdin);
        strtok(command, "\n");
        if (strcmp(command, "exit") == 0) {
            exit(EXIT_SUCCESS);
        }
        handleCommand(command);
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

void handleCommand(char command[MAX_COMMAND_SZ]) {
    pid_t pid;
    int flag = 0;
    int len = strlen(command);
    history[historyCount] = (char *) malloc(len);
    strcpy(history[historyCount], command);
    historyCount++;
    if (command[len - 1] == '&' && command[len - 2] == ' ') {
        flag = 1;
    }
    if ((pid = fork()) < 0) {
        printf("fork error\n");
    } else {
        if (pid == 0) {
            if (flag) { // background
                while (1) {
                    sleep(5);
                    printf("\ndone with %s\n", command);
                    exit(0);
                }
            } else { // foreground
                if (strcmp(command, "jobs") == 0) {
                    printf("handleJobs\n");
                } else if (strcmp(command, "history") == 0) {
                    int i = 0;
                    for (i = 0; i < historyCount; i++) {
                        printf("%s\n", history[i]);
                    }
                } else if (command[0] == 'c' && command[1] == 'd' && command[2] == ' ') {
                    printf("handleCD\n");
                }
                sleep(3);
                printf("done with %s\n", command);
                exit(0);
                /*
                 *
                 *         char **tokens = str_split(command, ' ');
                if (tokens) {
                    int i, cdCommand = 0;
                    if (strcmp(*tokens, "cd") == 0) {
                        printf("cdCommand\n");
                        cdCommand = 1;
                    }
                    for (i = 0; *(tokens + i); i++) {
                        printf("word=[%s]\n", *(tokens + i));
                    }
                    printf("\n");
                    freeTokens(tokens);

            }
                          * */
            }
        }
    }
    if (!flag) {
        if (waitpid(pid, NULL, 0) != pid) {
            printf("waitpid error\n");
        }
    }

}

void freeTokens(char **pString) {
    int i;
    for (i = 0; *(pString + i); i++) {
        free(*(pString + i));
    }
    free(pString);
}
