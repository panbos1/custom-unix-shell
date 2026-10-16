/*~~~~~~~~~~~~~~~~/
                  \
Assigment1        /
Panagiotis Mpermperidis\
csd4376                /
      16/10/2024       \
HY-345                 /
                      /
~~~~~~~~~~~~~~~~~~~~*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pwd.h>

#define MAX_CMD_LEN 1024    // max plithos entolwn/orismatwn
#define MAX_ARGS 100

void print_prompt() {                      //sinartisi gia emfanisi protropis 
    char cwd[MAX_CMD_LEN];
    char *user = getenv("USER");

    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("csd4376-hy345sh@%s:%s$ ", user, cwd);
    } else {
        perror("getcwd() error");
    }
}

void execute_command(char *cmd) {     //sinartisi gia aples entoles 
    char *args[MAX_ARGS];
    char *token = strtok(cmd, " \t\n");
    int i = 0;

    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL;

    if (args[0] == NULL) return; // Handle empty command

    if (strcmp(args[0], "exit") == 0) {
        exit(0);
    }

    pid_t pid = fork();

    if (pid == 0) {                      //diergasia gia paidi
        if (execvp(args[0], args) == -1) {
            perror("execvp error");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("fork error");
    } else {
        wait(NULL);         //mitriki diergasia 
    }
}

void handle_multiple_commands(char *input) {             //diaxorismos pollaplon entolwn
    char *cmd = strtok(input, ";");
    while (cmd != NULL) {
        execute_command(cmd);
        cmd = strtok(NULL, ";");
    }
}

void handle_redirection(char *cmd) {                //anakateuthinsi input/output
    char *args[MAX_ARGS];
    char *input_file = NULL, *output_file = NULL;
    int input_redir = 0, output_redir = 0, append = 0;
    char *token = strtok(cmd, " \t\n");
    int i = 0;

    while (token != NULL) {
        if (strcmp(token, "<") == 0) {
            input_redir = 1;
            input_file = strtok(NULL, " \t\n");
        } else if (strcmp(token, ">") == 0) {
            output_redir = 1;
            output_file = strtok(NULL, " \t\n");
        } else if (strcmp(token, ">>") == 0) {
            append = 1;
            output_file = strtok(NULL, " \t\n");
        } else {
            args[i++] = token;
        }
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL;

    pid_t pid = fork();

    if (pid == 0) {         // anakateuthinsi input
        if (input_redir) {
            int fd_in = open(input_file, O_RDONLY);
            if (fd_in < 0) {
                perror("open input error");
                exit(EXIT_FAILURE);
            }
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }

        if (output_redir) {      //anakateuthisnsi gia to output
            int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd_out < 0) {
                perror("open output error");
                exit(EXIT_FAILURE);
            }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        } else if (append) {
            int fd_out = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd_out < 0) {
                perror("open output error");
                exit(EXIT_FAILURE);
            }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }

        if (execvp(args[0], args) == -1) {
            perror("execvp error");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("fork error");
    } else {
        wait(NULL);
    }
}

void handle_pipes(char *cmd) {   //pipes 
    char *commands[2];
    int fd[2];
    pipe(fd);
    
    commands[0] = strtok(cmd, "|");
    commands[1] = strtok(NULL, "|");

    pid_t pid1 = fork();
    if (pid1 == 0) {			//gia child1
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        execute_command(commands[0]);
        exit(EXIT_FAILURE);
    }

    pid_t pid2 = fork();           //gia child2
    if (pid2 == 0) {
        dup2(fd[0], STDIN_FILENO);
        close(fd[1]);
        close(fd[0]);
        execute_command(commands[1]);
        exit(EXIT_FAILURE);
    }

    close(fd[0]);
    close(fd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

void interpret_input(char *input) {   //input kai elegxos pipes 
    if (strchr(input, '|')) {
        handle_pipes(input);
    } else if (strchr(input, '>') || strchr(input, '<') || strstr(input, ">>")) {
    handle_redirection(input);    
	} else {
        handle_multiple_commands(input);
    }
}

int main() {
    char input[MAX_CMD_LEN];

    while (1) {
        print_prompt();
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        interpret_input(input);
    }

    return 0;
}
