/**
 * A shell is a command-line interpreter that reads user input and
executes commands.

Build a Unix shell ( mysh ) that supports:
• Interactive command execution with prompt
• Built-in commands: exit, cd, pwd
• External command execution via fork() and exec()
• I/O redirection: > (truncate), » (append), < (input)
• Background execution: &

Shell follows REPL pattern (Read, Parse, Execute, Loop)

PSEUDOCODE
while (true):
    print prompt
    if fgets == NULL → break
    strip newline
    parse input
    if builtin → handle
    else:
        fork
        child → execvp
        parent → wait or not

References: 
Advanced Programming in the UNIX Environment by W. Richard Stevens
stackoverflow
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h> 
#include <fcntl.h> //handles file control operations  
#include <signal.h>

#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGS 100
#define MAX_PATH_LENGTH 4096

static void sig_int(int signo) {
    printf("\n");
}

int main(){

    char input[MAX_COMMAND_LENGTH];
    int status = 0;

    signal(SIGINT, sig_int); //register signal handler for SIGINT
    while (1){
        //PROMPT
        printf("\nmysh> ");
        fflush(stdout);

        //READ
        if (fgets(input, sizeof(input), stdin) == NULL){
            printf("\n");
            break; //handle EOF (Ctrl+D)
        }

        input[strcspn(input, "\n")] = 0; //strip newline character from input string

        //PARSE
        //The shell parses this null-terminated C string and breaks it up into separate command-line arguments for the command.
        int i = 0; //i is the index of the last argument
        int background = 0;
        char *args[MAX_ARGS], *ptr = strtok(input, " "); //strtok to tokenize the input string into separate arguments based on spaces
        
        while (ptr != NULL && i < MAX_ARGS - 1){
            args[i++] = ptr;
            ptr = strtok(NULL, " ");
        }
        args[i] = NULL; //null terminate the args array

        if (args[0] == NULL) {
            continue; //just skip if no command is entered
        }

        //BUILTIN COMMANDS
        if (strcmp(args[0], "exit") == 0) {
            exit(0);
        /**
         * if command == "cd":
            if no argument:
                path = HOME environment variable
            else:
                path = args[1]

            if chdir(path) fails:
                print error
            continue shell loop
         */
        } else if (strcmp(args[0], "cd") == 0) {
            char *path = args[1];
            if (path== NULL) { 
                path = getenv("HOME");
            }
            if (path == NULL) {
                fprintf(stderr, "cd: HOME not set\n");
            } else if (chdir(path) != 0) {
                perror("mysh: cd error");
            }
            continue;
        } else if (strcmp(args[0], "pwd") == 0) {
            char cwd[MAX_PATH_LENGTH];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
            } else {
                perror("getcwd error.");
            }
            continue;
        }
        
        //BACKGROUND EXECUTION
        //if the last character contains & -> background execution
        if (i > 0 && strcmp(args[i-1], "&") == 0){
            background = 1;
            args[i-1] = NULL; //remove & from args
        }

        if (signal(SIGINT, sig_int) == SIG_ERR){
            perror("signal error.");
        }
        // handle EOF (Ctrl+D)
        //EXECUTE EXTERNAL COMMANDS
        // read command line input
        pid_t process_id = fork();
        if (process_id < 0){  //OS cannot create new process if it reached max process limit
            perror("fork error.");
        } else if (process_id == 0){
            //child process
            for (int j = 0; args[j] != NULL; j++) {
                if (strcmp(args[j], ">") == 0) {
                    int fd = open(args[j+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd < 0) { perror("open"); exit(1); }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                    args[j] = NULL; 
                } else if (strcmp(args[j], ">>") == 0) { // Check args[j], not j+1
                    int fd = open(args[j+1], O_WRONLY | O_CREAT | O_APPEND, 0644);
                    if (fd < 0) { perror("open"); exit(1); }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                    args[j] = NULL;
                } else if (strcmp(args[j], "<") == 0) { // Check args[j], not j+1
                    int fd = open(args[j+1], O_RDONLY);
                    if (fd >= 0) {
                        dup2(fd, STDIN_FILENO);
                        close(fd);
                    } 
                    args[j] = NULL;
                }
            }
            execvp(args[0], args); 
            perror("exec error");
            exit(127);
        } else {
            //parent process
            if (!background){
                waitpid(process_id, &status, 0); //wait for child process to finish
            } else {
                printf("[Process running in background, PID: %d]\n", process_id);
                }
            }
        }
    return(status);
}