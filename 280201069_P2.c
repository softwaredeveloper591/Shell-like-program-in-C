#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h> 
#include <sys/wait.h>

#define MAX_COMMAND_LENGTH 100
#define MAX_QUEUE_SIZE 10
#define MAX_ARGS 10


// BEFORE MAIN I DEFINED AN ARRAY QUEUE 

typedef struct {
    char* items[MAX_QUEUE_SIZE];
    int front;
    int rear;
    int size;
} Queue;

void initializeQueue(Queue *q) {
    q->front = 0;
    q->rear = -1;
    q->size = 0;
}

int isFull(Queue *q) {
    return q->size == MAX_QUEUE_SIZE;
}

int isEmpty(Queue *q) {
    return q->size == 0;
}
char* dequeue(Queue *q) {
    if (isEmpty(q)) {
        printf("Queue is empty!\n");
        return NULL;
    }
    char *item = q->items[q->front];
    q->front = (q->front + 1) % MAX_QUEUE_SIZE;
    q->size--;
    return item;
}

// THE IMPORTAN DETAIL IS HERE, I INTEGRATED DEQUEUE INTO ENQUEUE SO THAT I DON'T NEED TO EMPTY THE QUEUE.
void enqueue(Queue *q, char *item) {
    if (isFull(q)) {
        // If the queue is full, dequeue the oldest item first to keep the last 10
        char *oldItem = dequeue(q);
        free(oldItem);
    }

    q->rear = (q->rear + 1) % MAX_QUEUE_SIZE; 
    q->items[q->rear] = strdup(item); 
    if (!isFull(q)) {
        q->size++;
    }
}


void printQueue(Queue *q) {
    for (int i = 0; i < q->size; i++) {
        int index = (q->front + i) % MAX_QUEUE_SIZE;
        printf("[%d] %s\n",i+1, q->items[index]);
    }
}

int main() {
    char command[MAX_COMMAND_LENGTH];   
    char savedCommand[MAX_COMMAND_LENGTH]; //since I will separate "command" variable into its tokens with strtok, it'll be damaged so I need to copy it into savedCommand 
    char *args[MAX_ARGS];  //this is the array that I am keeping tokens of "command"
    char *token;
    int i;
    char lastArgument[20];  //this is for "&" symbol for background instructions.

    size_t buffer_size = 1024;   // this buffer is needed to get "pwd" command with "$HOME"
    char *buffer = malloc(buffer_size);

    Queue q;
    initializeQueue(&q);
    

    while (1) {
        printf("myshell> ");
        fgets(command, MAX_COMMAND_LENGTH, stdin);  // we are getting the input string with a limited length and write it to command
        command[strcspn(command, "\n")] = '\0'; //in oder to get rid of "end" string.
        
        if (strcmp(command,"")==0) {  // if we didn't get rid of null termination above, this strcmp function would not returns 0 in case user press only enter.
            continue;                   // if only enter is pressed, then continue as a normal shell does.
        }
        strcpy(savedCommand, command);  // we are about to start separating the command into its tokens, copy it into savedCommand
        enqueue(&q, command);

        i = 0;       // within this while block I am separating the command into its tokens and assigning them to args array
        token = strtok(command, " ");
        while (token != NULL && i < MAX_ARGS - 1) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;
        strcpy(lastArgument, args[i-1]); // I will need lastArgument for & operand for background instructions
        if(strcmp(lastArgument,"&")==0){    // I need to clean this symbol from the argument to execute the command with execvp
            args[i-1]=NULL;
        }
        
        // if only custom commands are given then enter this if block
        if (strcmp(args[0], "pwd") == 0 || strcmp(args[0], "cd") == 0 || strcmp(args[0], "exit") == 0 || strcmp(args[0], "history") == 0){
            if (strcmp(command, "exit") == 0) {
                exit(0);
            }


            if (strcmp(command, "history") == 0) {
                printQueue(&q);
            }

            if (strcmp(args[0], "pwd") == 0) {

                if (buffer == NULL) {
                    perror("Failed to allocate memory");

                }

                // Get the current working directory
                if (getcwd(buffer, buffer_size) != NULL) {
                    printf("%s\n", buffer);
                } else {
                    perror("Failed for pwd");
                }
                //we need to free the buffer for next pwds
                free(buffer);  
            }

            if (strcmp(args[0], "cd") == 0) {
                if (args[1] == NULL) { // if there is no argument given, then change directory to home directory.
                    char *home = getenv("HOME");                    
                    if (home != NULL) {
                        if (chdir(home) != 0){
                            perror("Failed to change directory to home");
                        }
                    } 
                    else {
                        printf("Error: HOME environment variable not set.\n");
                    }
                } else {
                    //if arguemnt is provided then change to it
                    if (chdir(args[1]) != 0) {  //if chdir function return error
                        perror("Failed to change directory");
                    }
                }
            }

        }
        //if command contains this symbol
        else if(strchr(savedCommand, '|')) {
                int pipefd[2];  // Array to hold the file descriptors for the pipe
                pid_t pid1, pid2;
                char *first_cmd[100];  //for the part before |
                char *second_cmd[100]; //for the part after |
                char *token1;
                char *token2;
                char *token3;
                int i = 0;

                // Split the command  '|'
                token1 = strtok(savedCommand, "|");
                token3=  strtok(NULL, "|");
                token2 = strtok(token1, " ");
                while (token2 != NULL && i < 99) {
                    first_cmd[i++] = token2;
                    token2 = strtok(NULL, " ");
                }
                first_cmd[i] = NULL;

                i = 0;
                token2 = strtok(token3, " ");
                while (token2 != NULL && i < 99) {
                    second_cmd[i++] = token2;
                    token2 = strtok(NULL, " ");
                }
                second_cmd[i] = NULL;
                // here we create a pipe
                if (pipe(pipefd) == -1) {
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }

                pid1 = fork();
                if (pid1 == -1) {
                    perror("fork");
                    exit(EXIT_FAILURE);
                }
                if (pid1 == 0) {
                    dup2(pipefd[1], STDOUT_FILENO);  // now we need to Redirect stdout to the pipe
                    close(pipefd[0]);  // we Close the read end of the pipe so that we only write
                    close(pipefd[1]);
                    execvp(first_cmd[0], first_cmd); 
                    perror("execvp");
                    exit(EXIT_FAILURE);
                }

                pid2 = fork();
                if (pid2 == -1) {
                    perror("fork");
                    exit(EXIT_FAILURE);
                }
                if (pid2 == 0) {
                    dup2(pipefd[0], STDIN_FILENO);  // we need to redirect stdin to the pipe
                    close(pipefd[1]);  // we Close the write end of the pipe to only read
                    close(pipefd[0]);
                    execvp(second_cmd[0], second_cmd);
                    perror("execvp");
                    exit(EXIT_FAILURE);
                }

                close(pipefd[0]);
                close(pipefd[1]);
                
                //we are waiting for processes to finish as a shell here
                wait(NULL);
                wait(NULL);
            }

        // if the command contains && symbol
        else if(strstr(savedCommand,"&&")){
            // I used the same design and code for seperating the commands into tokens.
            pid_t pid3, pid4;
            char *first_cmd1[100];
            char *second_cmd1[100];
            char *token10;
            char *token20;
            char *token30;
            int in = 0;
            token10 = strtok(savedCommand, "&&");
            token30=  strtok(NULL, "&&");
            token20 = strtok(token10, " ");

            while (token20 != NULL && in < 99) {
                first_cmd1[in++] = token20;
                token20 = strtok(NULL, " ");
            }
            first_cmd1[in] = NULL;
            in = 0;
            token20 = strtok(token30, " ");
            while (token20 != NULL && in < 99) {
                second_cmd1[in++] = token20;
                token20 = strtok(NULL, " ");
            }
            second_cmd1[in] = NULL;
        
            pid3 = fork();
            if (pid3 == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            }
            else if (pid3 == 0) {
                execvp(first_cmd1[0], first_cmd1); 
                perror("execvp");
                exit(EXIT_FAILURE);
            }
            else{
                int status;
                waitpid(pid3, &status, 0);  // we are Waiting for first process to finish and if it exit without error then to the second command
                if (WIFEXITED(status)) {
                    int exit_status = WEXITSTATUS(status);
                    if (exit_status == 0) {
                        //if It returns with success then second
                        pid4 = fork();
                        if (pid4 == -1) {
                            perror("fork");
                            exit(EXIT_FAILURE);
                        }
                        if (pid4 == 0) {
                            execvp(second_cmd1[0], second_cmd1);
                            perror("execvp");
                            exit(EXIT_FAILURE);
                        }
                        else{
                            wait(NULL);
                        }
                    } else {
                        printf("First Command failed to execute.\n");
                    }
                }
                
            }

    
        }

        // IN THIS ELSE BLOCK I IMPLEMENTED THE PARTS OUTSIDE "|" , "&&" , PWD, CD, EXIT, AND HISTORY COMMANDS, WHICH ARE STANDART SYSTEM CALLS
        else{   
            pid_t pid = fork(); 
            if (pid == -1) {
                perror("fork");
                return 1;
            } else if (pid == 0) {
                execvp(args[0], args); 
                perror("execvp");
               exit(EXIT_FAILURE);  
            } else {
                //IF OUR LAST ARGUMENT IN ARGS IS & THEN WE NEED TO EXECUTE IT IN BACKGROUND.
                if(strcmp(lastArgument,"&")!=0){
                    wait(NULL); 
                }
                else{
                    printf("%d\n",pid);
                }

            }
        }

    }

    return 0;
}
