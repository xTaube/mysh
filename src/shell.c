#include "parsers.h"
#include "readline.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void process_line(char *line) {
  if (strcmp(line, "exit") == 0) {
    exit(EXIT_SUCCESS);
  }

  char **args = parse_line_arguments(line);
  if (!args) {
    return;
  }

  if (strcmp(args[0], "cd") == 0) {
    if (args[1] == NULL) {
      fprintf(stderr, "mysh \"cd\" expects argument\n");
    } else if (chdir(args[1]) != 0) {
      perror("mysh");
    }

  } else {
    int rc = fork();
    if (rc < 0) {
      perror("fork error.");
      exit(EXIT_FAILURE);
    } else if (rc == 0) { // child process
      if (execvp(args[0], args) == -1) {
        fprintf(stderr, "mysh: command not found: %s\n", args[0]);
        exit(EXIT_FAILURE);
      };
    } else {
      wait(NULL);
    }
  }
  free(args);
}

int run_shell() {
  char *line = NULL;
  while ((line = readline("mysh> ")) != NULL) {
    add_history(line);

    process_line(line);
    free(line);
  }

  return EXIT_SUCCESS;
}
