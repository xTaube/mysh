#include "parsers.h"
#include "readline.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void execute_single_command(char *command) {
  char **args = parse_command_arguments(command);
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
    int pid = fork();
    if (pid < 0) {
      perror("fork error.");
      exit(EXIT_FAILURE);
    } else if (pid == 0) { // child process
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

static void execute_pipeline(char **commands, size_t commands_num) {
  int pipefd[2] = {0};
  int in_fd = STDIN_FILENO;

  for (size_t i = 0; i < commands_num; i++) {
    if (pipe(pipefd) < 0) {
      perror("pipe");
      exit(EXIT_FAILURE);
    };

    pid_t pid = fork();
    if (pid < 0) {
      perror("fork error.");
      exit(EXIT_FAILURE);
    }

    if (pid == 0) {
      dup2(in_fd, STDIN_FILENO);
      if (i != commands_num - 1)
        dup2(pipefd[1], STDOUT_FILENO);

      close(pipefd[0]);

      char **args = parse_command_arguments(commands[i]);
      if (execvp(args[0], args) == -1) {
        fprintf(stderr, "mysh: command not found: %s\n", args[0]);
        exit(EXIT_FAILURE);
      };
    }

    close(pipefd[1]);
    in_fd = pipefd[0];
  }

  while (wait(NULL) > 0)
    ;
}

static void process_line(char *line) {
  if (strcmp(line, "exit") == 0) {
    exit(EXIT_SUCCESS);
  }

  char **commands = NULL;
  size_t commands_num = parse_line_commands(line, &commands);
  if (commands_num == 0)
    return;

  if (commands_num == 1)
    execute_single_command(commands[0]);
  else {
    execute_pipeline(commands, commands_num);
  }
  free(commands);
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
