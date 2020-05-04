#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <assert.h>

#define MAX_COMMAND_SZ 256

void handleForkCommand(char command[MAX_COMMAND_SZ]);

char **str_split(char *a_str, const char a_delim);

void freeTokens(char **pString);

void addHistory(char command[256], __pid_t pid, int background);

void print(int historyOrJobs);

typedef struct History {
    pid_t pid;
    char command[100];
} History;

History history[100];
int historyCount = 0;

int main() {
    char command[MAX_COMMAND_SZ];
    char prevPath[200]; // string representing the previous path
    getcwd(prevPath, 200); // initialize prevPath
    while (1) {
        printf("> "); // prompt
        fgets(command, MAX_COMMAND_SZ, stdin);
        if (strcmp(command, "\n") == 0) { // if it's empty string
            continue;
        }
        strtok(command, "\n");
        if (strcmp(command, "exit") == 0) {
            printf("%d\n", getpid());
            exit(EXIT_SUCCESS);
        } else if (strcmp(command, "history") == 0) {
            addHistory(command, getpid(), 0);
            print(1);
            // printHistory();
        } else if (strcmp(command, "jobs") == 0) {
            addHistory(command, getpid(), 0);
            print(0);
            // printJobs();
        } else if (command[0] == 'c' && command[1] == 'd') {
            if (strlen(command) == 2) { // if it's just "cd"
                addHistory(command, getpid(), 0);
                continue;
            } else if (command[2] != ' ') {
                handleForkCommand(command);
            }
            char currentPath[200];
            getcwd(currentPath, 200); // save currentPath
            addHistory(command, getpid(), 0);
            printf("%d\n", getpid());
            char **commandToExec = str_split(command, ' ');
            int i = 0;
            while (*(commandToExec + i)) { // counting words in the command
                i++;
            }
            if (i > 2) { // if more than 2, illegal
                fprintf(stderr, "Error: Too many arguments\n");
            } else { // legal
                char *wordAfterCD = commandToExec[1];
                if (wordAfterCD[0] != '\0') { // means there is a string
                    if (strlen(wordAfterCD) >= 2 && wordAfterCD[0] == '.' && wordAfterCD[1] == '.') {
                        getcwd(prevPath, 200); // save prevPath
                        chdir(wordAfterCD);
                    } else if (strcmp(wordAfterCD, "-") == 0) {
                        chdir(prevPath);
                        strcpy(prevPath, currentPath); // save prevPath
                    } else if (strcmp(wordAfterCD, "~") == 0) {
                        getcwd(prevPath, 200); // save prevPath
                        chdir(getenv("HOME"));
                    } else {
                        int ret;
                        getcwd(prevPath, 200); // save prevPath
                        ret = chdir(wordAfterCD);
                        if (ret == -1) {
                            fprintf(stderr, "Error: No such file of directory\n");
                        }
                    }
                    // printf("%s\n", getcwd(currentPath, 200));
                }
            }

        } else {
            handleForkCommand(command);
        }
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

// adding history
void addHistory(char command[256], __pid_t pid, int background) {
    history[historyCount].pid = pid;
    strcpy(history[historyCount].command, command);
    historyCount++;
}

// the function removes quotes from string
char *removeQuotes(char *line) {
    int j = 0, i = 0;
    for (i = 0; i < strlen(line); i++) {
        if (line[i] != '"' && line[i] != '\\') {
            line[j++] = line[i];
        } else if (line[i + 1] == '"' && line[i] == '\\') {
            line[j++] = '"';
        } else if (line[i + 1] != '"' && line[i] == '\\') {
            line[j++] = '\\';
        }
    }
    if (j > 0) { line[j] = 0; }
    return line;
}

// handles commands that requires fork
void handleForkCommand(char command[MAX_COMMAND_SZ]) {
    pid_t pid;
    int background = 0;
    int len = strlen(command);
    if (command[len - 1] == '&' && command[len - 2] == ' ') {
        background = 1;
        int idxToDel = len - 1;
        memmove(&command[idxToDel], &command[idxToDel + 1], strlen(command) - idxToDel);
        idxToDel = len - 2;
        memmove(&command[idxToDel], &command[idxToDel + 1], strlen(command) - idxToDel);
        len -= 2;
    }

    if ((pid = fork()) < 0) {
        printf("fork error\n");
    } else if (pid == 0) { // here the child process will "go"
        char tempCommand[MAX_COMMAND_SZ];
        strcpy(tempCommand, command);
        char **commandToExec = str_split(tempCommand, ' '); // splitting words of the command to array of strings
        if (strcmp(commandToExec[0], "echo") == 0) { // we want to get rid of quotes
            strcpy(tempCommand, command);
            int j, i;
            for (j = 0; j < 5; j++) {
                for (i = 1; i < len; i++) {
                    tempCommand[i - 1] = tempCommand[i];
                }
                int idxToDel = len - 1;
                memmove(&tempCommand[idxToDel], &tempCommand[idxToDel + 1], strlen(tempCommand) - idxToDel);
                len--;
            }

            char *new_str = malloc(strlen("echo ") + strlen(removeQuotes(tempCommand)) + 1);
            new_str[0] = '\0';   // ensures the memory is an empty string
            strcat(new_str, "echo ");
            strcat(new_str, removeQuotes(tempCommand));
            commandToExec = str_split(new_str, ' ');
        }
        if (execvp(commandToExec[0], commandToExec) == -1) {
            fprintf(stderr, "Error in system call\n");
        }
        freeTokens(commandToExec);
        exit(0);
    } else { // here the parent process will "go"
        addHistory(command, pid, background);
        printf("%d\n", pid);
        if (!background) { // don't wait for background to finish
            if (waitpid(pid, NULL, 0) != pid) {
                printf("waitpid error\n");
            }
        }
    }
}

// function splits string by delimiter specified return double string array
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

// free double string array
void freeTokens(char **pString) {
    int i;
    for (i = 0; *(pString + i); i++) {
        free(*(pString + i));
    }
    free(pString);
}
