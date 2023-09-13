#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_CMD_LINE_ARGS  128
//look at slides for parse code
int min(int a, int b) { return a < b ? a : b; }


int parse(char* s, char* argv[]) {
  char* filename;
  char in_or_out;
  while (*s != '\0'){
    while (isspace(*s)){
      *s++ = '\0';
    }
    if (*s =='<'){
      in_or_out = *s;
      while (isspace(*s)){
        ++s;
      }
      filename = s;
      while (!isspace(*s)){
        ++s;
      }
      redirect_input(filename);
    }

  }
  int c = 0;
  while (*s != '\0') {
    while (isspace(*s)) {       // skip leading whitespace
      ++s;
    }
    if (*s != '\0') {           // found a token
      argv[c++] = s;            // save ptr to beginning of token
    }
    while (!isspace(*s) && (*s != '\0')) {  // scan token until whitespace or end
      ++s;
    }
    if (*s != '\0') {           // end of token?  put a \0, and keep going
      *s = '\0';
      ++s;
    }
  } 
  return c;   // int argc
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
    // if ((ret = execlp("cal", "cal", NULL)) < 0) {  // can do it arg by arg, ending in NULL
		// TODO: fill in code for execvp(...)
		printf("You should have called execvp(...)\n");
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
      // TODO
    } else { execute(input); }
  }
  
  printf("\t\t...exiting\n");
  return 0;
}
