#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/wait.h> // Include this header for wait and waitpid
#define MAX_CMD_LINE_ARGS  128

#define MAX_HISTORY_SIZE   10  // Adjust this as needed

char history[MAX_HISTORY_SIZE][BUFSIZ];
int history_index = 0;


int min(int a, int b) { return a < b ? a : b; }

void redirect_input(const char* filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open filename in redirect_input failed");
        exit(-1);
    }
    dup2(fd, STDIN_FILENO); // Redirect standard input to the file descriptor fd
    close(fd); // Close the file descriptor as it's no longer needed
}

void redirect_output(const char* filename, bool append) {
    int flags = O_WRONLY;
    flags |= append ? O_APPEND : O_CREAT;

    int fd = open(filename, flags, 0666); // You can adjust file permissions as needed
    if (fd == -1) {
        perror("open failure in redirect_output");
        exit(-2);
    }
    dup2(fd, STDOUT_FILENO); // Redirect standard output to the file descriptor fd
    close(fd); // Close the file descriptor as it's no longer needed
}

// break a string into its tokens, putting a \0 between each token
//   save the beginning of each string in a string of char *'s (ptrs to chars)
bool isBackground = false; // Global variable to hold background flag

int parse(char* s, char* argv[]) {
    const char break_chars[] = " \t\n";
    char* p;
    int c = 0;
    isBackground = false; // Reset for each new command
    
    p = strtok(s, break_chars);
    while (p != NULL) {
        argv[c] = p;
        c++;
        p = strtok(NULL, break_chars);
    }

    // Check if the last argument is an ampersand
    if (strcmp(argv[c - 1], "&") == 0) {
        isBackground = true;
        argv[c - 1] = NULL; // Replace "&" with NULL to make it execvp compatible
        c--; // Reduce argument count
    } else {
        argv[c] = NULL; // execvp expects NULL as the last value
    }
    
    return c;
}

// execute a single shell command, with its command line arguments
//     the shell command should start with the name of the command
// execute a single shell command, with its command line arguments
int execute(char* input) {
    printf("Attempting to run: %s\n", input); // Debugging

    // Initialize command arguments
    char* shell_argv[MAX_CMD_LINE_ARGS];
    memset(shell_argv, 0, MAX_CMD_LINE_ARGS * sizeof(char*));

    // Parse the input
    parse(input, shell_argv);

    // Fork a new process
    pid_t pid = fork();

    if (pid < 0) {
        // Fork failed
        perror("Fork failed");
        return 1;
    }

    if (pid == 0) {
        // This block will be executed by the child process

        // Execute the command
        if (execvp(shell_argv[0], shell_argv) < 0) {
            perror("execvp failed");
            exit(EXIT_FAILURE); // terminate the child process if exec fails
        }
    } else {
        // This block will be executed by the parent
        if (!isBackground) {
            // If the process is not supposed to run in the background, wait for it to complete
            int status;
            while (wait(&status) != pid);
        }
    }

    return 0;
}


void execute_child_process(char* command) {
  char* args[MAX_CMD_LINE_ARGS];
  char* output_file = NULL;
  char* input_file = NULL;
  int arg_index = 0;

  // Tokenize the command into separate arguments
  char* token = strtok(command, " ");
  while (token != NULL && arg_index < MAX_CMD_LINE_ARGS - 1) {
    if (strcmp(token, ">") == 0) {
      token = strtok(NULL, " ");
      if (token == NULL) {
        fprintf(stderr, "Syntax error: missing file after >\n");
        return;
      }
      output_file = token;
    } else if (strcmp(token, "<") == 0) {
      token = strtok(NULL, " ");
      if (token == NULL) {
        fprintf(stderr, "Syntax error: missing file after <\n");
        return;
      }
      input_file = token;
    } else {
      args[arg_index++] = token;
    }
    token = strtok(NULL, " ");
  }
  args[arg_index] = NULL;

  // Fork a child process
  pid_t pid = fork();

  if (pid < 0) {
    perror("fork");
    return;
  } else if (pid == 0) {
    // Child process
    if (output_file != NULL) {
      int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
      if (fd == -1) {
        perror("open");
        exit(1);
      }
      dup2(fd, STDOUT_FILENO);
      close(fd);
    }

    if (input_file != NULL) {
      int fd = open(input_file, O_RDONLY);
      if (fd == -1) {
        perror("open");
        exit(1);
      }
      dup2(fd, STDIN_FILENO);
      close(fd);
    }

    execvp(args[0], args);
    perror("execvp");
    exit(1);
  } else {
    // Parent process
    wait(NULL);
  }
}


int commands_executed = 0;
void execute_with_pipe(char* input) {
    char *left_cmd = strtok(input, "|");
    char *right_cmd = strtok(NULL, "|");

    if (right_cmd == NULL) {
        execute(left_cmd);  // If there's no pipe, just execute the command normally.
        return;
    }

    int pipefd[2];
    pipe(pipefd);  // Create the pipe
    pid_t pid1, pid2;

    if ((pid1 = fork()) == 0) {
        close(pipefd[0]);  // Close the read end of the pipe in the first child
        dup2(pipefd[1], STDOUT_FILENO);
        execute(left_cmd);
        exit(0);
    }

    if ((pid2 = fork()) == 0) {
        close(pipefd[1]);  // Close the write end of the pipe in the second child
        dup2(pipefd[0], STDIN_FILENO);
        execute(right_cmd);
        exit(0);
    }

    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}


int main(void) {
  char input[BUFSIZ];
  bool finished = false;

  while (!finished) {
    memset(input, 0, BUFSIZ * sizeof(char));

    printf("osh > ");
    fflush(stdout);

    if ((fgets(input, BUFSIZ, stdin)) == NULL) {
      fprintf(stderr, "no command entered\n");
      exit(1);
    }
    input[strlen(input) - 1] = '\0'; // Remove trailing newline

    if (strncmp(input, "exit", 4) == 0) {
      finished = true;
    } else if (strncmp(input, "!!", 2) == 0) {
      if (commands_executed == 0) {
        printf("No commands in history.\n");
      } else {
        int last_command_index = (history_index - 1 + MAX_HISTORY_SIZE) % MAX_HISTORY_SIZE;
        printf("Executing from history: %s\n", history[last_command_index]);
        execute(history[last_command_index]);
      }
    } else if (strchr(input, '|') != NULL) {  
        execute_with_pipe(input);
    } else {
      strncpy(history[history_index], input, min(strlen(input), BUFSIZ));
      history_index = (history_index + 1) % MAX_HISTORY_SIZE;
      commands_executed++;
      printf("Command to execute: %s\n", input); // Debugging

      // Check if the command contains '<' or '>', in which case it's a redirection command
      if (strchr(input, '<') != NULL || strchr(input, '>') != NULL) {
        execute_child_process(input);
      } else {
        execute(input);
      }
    }
  }

  printf("\t\t...exiting\n");
  return 0;
}

