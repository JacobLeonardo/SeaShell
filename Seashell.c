/**
 * @file SeaShell.c
 * @author Jacob Leonardo
 * @date April 9, 2025
 * @brief A simple shell program that can execute commands, handle input and output redirection, background processes, and pipe commands.
 * @version 1.0
*/


#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

void welcomeMessage();
void execCmd(char** parsed);
void inputRedirection(char** parsed, int input_file_pos);
void outputRedirection(char** parsed, int output_file_pos, int append);
void pipeCommands(char** parsed, int background, int output_redirection, int append, int input_redirection, int output_file_pos, int input_file_pos, int piped_cmd_pos);


/**
 * @brief Main function for the SeaShell program.
 * @return 0 on success, 1 on failure.
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line arguments.
 * @details Entry point of the Seashell program. Prints the welcome banner,
 * then enters an infinite loop to read user input, parse it into tokens,
 * and execute commands accordingly.
 */
int main(int argc, char* argv[]) {
    welcomeMessage();
    while (1) {
        char command[100];
        char* args[15];
        int should_run = 1;
        int n = 10 * sizeof(int);
        printf("\nSeaShell> ");
        fflush(stdout);

        if (!fgets(command, sizeof(command), stdin)) {
            break;
        }

        command[strcspn(command, "\n")] = 0;

        //Tokenize the input
        char* token = strtok(command, " ");
        int i = 0;
        while (token != NULL && i < 10) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        //Handle built-in commands
        if (args[0] != NULL && strcmp(args[0], "exit") == 0) {
            exit(1);
        }
        else if (args[0] != NULL && strcmp(args[0], "cd") == 0) {
            if (args[1] != NULL) {
                if (strcmp(args[1], "~") == 0) {
                    char* home = getenv("HOME");
                    if (chdir(home) == 0) {
                        printf("Changed directory to home.\n", home);
                    }
                    else {
                        perror("Error: Failed to change directory to home.\n");
                        exit(1);
                    }
                }
                else {
                    if (chdir(args[1]) != 0) {
                        perror("Error: Failed to change directory.\n");
                    }
                }
            }
        }
        else if (args[0] != NULL) {
            execCmd(args);
        }
    }
    return 0;
}



/**
 * @brief Prints a welcome message with the current date and time.
 */
void welcomeMessage() {
    time_t t;
    struct tm* tm_info;
    char dateBuffer[20];
    char timeBuffer[20];
    time(&t);
    tm_info = localtime(&t);
    strftime(dateBuffer, sizeof(dateBuffer), "%m/%d/%Y", tm_info);
    strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", tm_info);
    printf("**************************************************\n");
    printf("*             Welcome to SeaShell                *\n");
    printf("*                  Created by                    *\n");
    printf("*                Jacob Leonardo                  *\n");
    printf("*                                                *\n");
    printf("*                 Date: %s               *\n", dateBuffer);
    printf("*                 Time: %s                 *\n", timeBuffer);
    printf("**************************************************\n\n");
}

/**
     * @brief Executes a command with the given arguments.
     * @details This function handles the execution of a command, including background processes, input and output redirection, and piping. It forks a new process to execute the command and waits for it to complete unless it is a background process.
     * @param parsed The array of command-line arguments.
*/
void execCmd(char** parsed) {
    int background = 0;
    int append = 0;
    int output_redirection = 0;
    int input_redirection = 0;
    int input_file_pos = 0;
    int output_file_pos = 0;
    int pipe_fd[2];
    int is_pipe = 0;
    int piped_cmd_pos = 0;

    //Check for special characters
    for (int i = 0; i < 10; i++) {
        if (parsed[i] == NULL)
            break;
        if (strcmp(parsed[i], "&") == 0) {
            parsed[i] = NULL;
            background = 1;
            continue;
        }
        else if (strcmp(parsed[i], ">") == 0) {
            parsed[i] = NULL;
            output_redirection = 1;
            output_file_pos = i;
            continue;
        }
        else if (strcmp(parsed[i], ">>") == 0) {
            parsed[i] = NULL;
            append = 1;
            output_file_pos = i;
            continue;
        }
        else if (strcmp(parsed[i], "<") == 0) {
            parsed[i] = NULL;
            input_redirection = 1;
            input_file_pos = i;
            continue;
        }
        else if (strcmp(parsed[i], "|") == 0) {
            parsed[i] = NULL;
            is_pipe = 1;
            piped_cmd_pos = i + 1;
            pipe(pipe_fd);
            continue;
        }
    }

    //Handle piping
    if (is_pipe == 1) {
        pipeCommands(parsed, background, output_redirection, append, input_redirection, output_file_pos, input_file_pos, piped_cmd_pos);

    }
    else {

        //Fork and execute the command
        pid_t pid = fork();

        if (pid == -1) {
            printf("\nFailed forking child..");
            return;
        }
        else if (pid == 0) {
            if (output_redirection == 1 || append == 1) {
                outputRedirection(parsed, output_file_pos, append);
            }
            if (input_redirection == 1) {
                inputRedirection(parsed, input_file_pos);
            }

            if (execvp(parsed[0], parsed) < 0) {
                printf("\nCould not execute command..\n");
            }
            exit(0);
        }
        else {
            if (background == 1) {
                waitpid(pid, NULL, WNOHANG);
                return;
            }
            wait(NULL);
        }
    }
}


/**
 * @brief Handles input redirection for a command.
 * @param cmd The array of command-line arguments.
 * @param input_file_pos The position of the input file in the command array.
 * @details This function opens the input file and redirects standard input to it.
*/
void inputRedirection(char** cmd, int input_file_pos) {
    int fd = open(cmd[input_file_pos + 1], O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        exit(1);
    }
    dup2(fd, STDIN_FILENO);
    close(fd);
}

/**
 * @brief Handles output redirection for a command.
 * @param cmd The array of command-line arguments.
 * @param output_file_pos The position of the output file in the command array.
 * @param append A flag indicating whether to append to the output file or overwrite it.
 * @details This function opens the output file and redirects standard output to it. If the append flag is set, it appends to the file; otherwise, it overwrites the file.
*/
void outputRedirection(char** cmd, int output_file_pos, int append) {
    int fd;
    if (append) {
        fd = open(cmd[output_file_pos + 1], O_WRONLY | O_CREAT | O_APPEND, 0777);
    }
    else {
        fd = open(cmd[output_file_pos + 1], O_WRONLY | O_CREAT | O_TRUNC, 0777);
    }
    if (fd < 0) {
        perror("Error opening file");
        exit(1);
    }
    dup2(fd, STDOUT_FILENO);
    close(fd);
}

/**
 * @brief Handles piping between two commands.
 * @param parsed The array of command-line arguments.
 * @param background A flag indicating whether the command should run in the background.
 * @param output_redirection A flag indicating whether output redirection is enabled.
 * @param append A flag indicating whether to append to the output file.
 * @param input_redirection A flag indicating whether input redirection is enabled.
 * @param output_file_pos The position of the output file in the command array.
 * @param input_file_pos The position of the input file in the command array.
 * @param piped_cmd_pos The position of the piped command in the command array.
 * @details This function creates a pipe and forks two child processes to execute the two commands. The first child process writes to the pipe, and the second child process reads from the pipe.
 * @note This function assumes that the command array contains two commands separated by a pipe character.
 * @note This function does not handle errors that occur during pipe creation or process creation.
*/
void pipeCommands(char** parsed, int background, int output_redirection, int append, int input_redirection, int output_file_pos, int input_file_pos, int piped_cmd_pos) {

    int pipe_fd[2];

    if (pipe(pipe_fd) < 0) {
        perror("Pipe creation failed");
        return;
    }

    pid_t pid1 = fork();

    if (pid1 < 0) {
        perror("Fork failed");
        return;
    }

    if (pid1 == 0) {
        //First child process
        close(pipe_fd[0]);
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[1]);

        if (input_redirection == 1) {
            inputRedirection(parsed, input_file_pos);
        }

        if (execvp(parsed[0], parsed) < 0) {
            perror("First command execution failed");
            exit(1);
        }
        exit(0);
    }
    else {

        pid_t pid2 = fork();

        if (pid2 < 0) {
            perror("Second fork failed");
            return;
        }

        if (pid2 == 0) {
            //Second child process
            close(pipe_fd[1]);
            dup2(pipe_fd[0], STDIN_FILENO);
            close(pipe_fd[0]);

            if (output_redirection == 1 || append == 1) {
                outputRedirection(parsed, output_file_pos, append);
            }

            if (execvp(parsed[piped_cmd_pos], &parsed[piped_cmd_pos]) < 0) {
                perror("Second command execution failed");
                exit(1);
            }
            exit(0);
        }
        else {
            //Parent process: wait for both child processes to complete
            close(pipe_fd[0]);
            close(pipe_fd[1]);

            if (background == 1) {
                return;
            }

            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0);
        }
    }
}


