#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#define MAX_CMD_LINE_ARGS  128

int min(int a, int b) { return a < b ? a : b; }

void redirect_input(const char* filename) { 
    int fd = open (filename, O_RDONLY);
    if (fd == -1) {  
        perror("open filename in redirect_input failed");
        exit(-1);
    }
    dup2(fd, STDIN_FILENO);
}

void redirect_output(const char* filename, bool append) { 
    int flags = O_WRONLY;
    flags |= append ? O_APPEND : O_CREAT;

    int fd = open (filename, flags);
    if (fd == -1) { 
        perror("open failure in redirect_output");
        exit(-2);
    }
    dup2(fd, STDOUT_FILENO);
}
// break a string into its tokens, putting a \0 between each token
//   save the beginning of each string in a string of char *'s (ptrs to chars)

int parse(char* p, char* argv[]) {
  char* filename;
  char in_or_out;
  bool append = false;
  int argc = 0;

  while (*p != '\0') { 
      while (*p != '\0' && isspace(*p)) { 
          *p++ = '\0';
      }
      if (*p == '<' || *p == '>') { 
          in_or_out = *p++;
          if (*p == '>') {++p; append = true; }   // >> means append to file
          while (*p != '\0' && isspace(*p)) {
              ++p;
          }
          filename = p;
          while (*p != '\0' && !isspace(*p)) { 
              ++p;
          }
          *p++ = '\0';
          in_or_out == '<' ? redirect_input(filename) : redirect_output(filename, append);
          continue; 
      }
      *argv++ = p;
      ++argc;
      while (*p != '\0' && !isspace(*p)) { 
        ++p; 
      }
  }
  *argv = NULL;
  return argc;
}


// execute a single shell command, with its command line arguments
//     the shell command should start with the name of the command
int execute(char* input) {
  int i = 0;
  char* shell_argv[MAX_CMD_LINE_ARGS];
  memset(shell_argv, 0, MAX_CMD_LINE_ARGS * sizeof(char));

  
  int shell_argc = parse(input, shell_argv);
  // printf("after parse, what is input: %s\n", input);      // check parser
  // printf("argc is: %d\n", shell_argc);
  // while (shell_argc > 0) {
  //   printf("argc: %d: %s\n", i, shell_argv[i]);
  //   --shell_argc;
  //   ++i;
  // }
  
  int status = 0;
  pid_t pid = fork();
  
  if (pid < 0) { fprintf(stderr, "Fork() failed\n"); }  // send to stderr
  else if (pid == 0) { // child
    int ret = 0;
    if ((ret = execvp(shell_argv[0], shell_argv)) < 0) {  // can do it arg by arg, ending in NULL
      exit(1);
    }
    exit(0);
  }
  else { // parent -----  don't wait if you are creating a daemon (background) process
    while (wait(&status) != pid) { }
  }


return 0;
}

int main(int argc, const char * argv[]) {
  char input[BUFSIZ];
  char last_input[BUFSIZ];  
  bool finished = false;

  memset(last_input, 0, BUFSIZ * sizeof(char));  
  while (!finished) {
    memset(input, 0, BUFSIZ * sizeof(char));

    printf("osh > ");
    fflush(stdout);

    if (strlen(input) > 0) {
      strncpy(last_input, input, min(strlen(input), BUFSIZ));
      memset(last_input, 0, BUFSIZ * sizeof(char));
    }

    if ((fgets(input, BUFSIZ, stdin)) == NULL) {   // or gets(input, BUFSIZ);
      fprintf(stderr, "no command entered\n");
      exit(1);
    }
    input[strlen(input) - 1] = '\0';          // wipe out newline at end of string
    // printf("input was: '%s'\n", input);
    // printf("last_input was: '%s'\n", last_input);
    if (strncmp(input, "exit", 4) == 0) {   // only compare first 4 letters
      finished = true;
    } else if (strncmp(input, "!!", 2) == 0) { // check for history command
      // TODO:  Not sure if this is the answer
      execute(last_input);
      } else { execute(input); }
  }
  
  printf("\t\t...exiting\n");
  return 0;
}
shell-2-1.c
4 KB
