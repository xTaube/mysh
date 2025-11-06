#include "parsers.h"
#include "attrs.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ATTR_PURE
static size_t count_args(const char *line) {
  size_t count = 0;
  const char *p = line;
  bool in_arg = false;

  while (*p != '\0') {
    if (in_arg && isspace((unsigned char)*p)) {
      in_arg = false;
    } else if (!in_arg && !isspace((unsigned char)*p)) {
      count++;
      in_arg = true;
    }
    p++;
  }

  return count;
}

ATTR_PURE
static size_t count_commands(const char *line) {
  if (*line == '\0')
    return 0;

  size_t count = 1;
  const char *p = line;
  while (*p != '\0') {
    if (*p == '|')
      count++;
    p++;
  }

  return count;
}

size_t parse_line_commands(char *line, char **commands[]) {
  size_t commands_num = count_commands(line);
  if (commands_num < 1)
    return commands_num;

  *commands = malloc((commands_num) * sizeof(char *));
  if (!*commands) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  char *token = strtok(line, "|");
  for (size_t i = 0; i < commands_num; i++) {
    (*commands)[i] = token;
    token = strtok(NULL, "|");
  }

  return commands_num;
}

ATTR_NODISCARD ATTR_ALLOC char **parse_command_arguments(char *command) {
  size_t args_num = count_args(command);

  if (args_num < 1) {
    return NULL;
  }

  char **args = malloc((args_num + 1) * sizeof(char *));
  if (!args) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  char *p = command;

  bool in_arg = false;
  size_t arg_num = 0;
  while (*p != '\0') {
    if (!in_arg && !isspace((unsigned char)*p)) {
      args[arg_num] = p;
      arg_num++;
      in_arg = true;
    } else if (in_arg && isspace((unsigned char)*p)) {
      in_arg = false;
      *p = '\0';
    }
    p++;
  }

  args[args_num] = NULL;
  return args;
}
