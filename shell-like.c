#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#define MAX_COMMAND_LENGTH 1024

int main() {
    char command[MAX_COMMAND_LENGTH];
    char *cmd;       // To store the command itself
    char *arg;       // To store the argument to the command

    size_t buffer_size = 1024; // You can adjust the size as needed
    char *buffer = malloc(buffer_size);

    while (1) {
        // Display the shell prompt
        printf("myshell> ");

        // Get input from the user
        if (fgets(command, MAX_COMMAND_LENGTH, stdin) == NULL) {
            // If there's no input (e.g., EOF), exit the loop
            break;
        }

        // Remove the newline character from the end of input
        command[strcspn(command, "\n")] = '\0';
        // Split the command into command and argument
        cmd = strtok(command, " ");
        arg = strtok(NULL, " ");

        printf("command: %s and argument(s): %s\n", cmd,arg);

        // Check for the exit command to quit the shell
        if (strcmp(command, "exit") == 0) {
            break;
        }

        if (strcmp(command, "pwd") == 0) {
            if (buffer == NULL) {
                perror("Failed to allocate memory");
                return 1;
            }

            // Get the current working directory
            if (getcwd(buffer, buffer_size) != NULL) {
                printf("Current Working Directory: %s\n", buffer);
            } else {
                perror("Failed to get current working directory");
            }

            // Free the allocated memory
            free(buffer);
                }

        

        // Handling the 'cd' command
        if (strcmp(cmd, "cd") == 0) {
            if (arg == NULL) {
                printf("cd requires a directory path as an argument.\n");
            } else {
                if (chdir(arg) == 0) {
                    printf("Changed directory to %s\n", arg);
                } else {
                    printf("Failed to change directory: %s\n", strerror(errno));
                }
            }
        } else {
            // Echo the command back to the user if it's not recognized
            printf("Command not recognized: %s\n", command);
        }
    }

    return 0;
}
