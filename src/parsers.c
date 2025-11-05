#include "parsers.h"
#include "attrs.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

ATTR_PURE
static size_t count_args(char *line) {
  size_t count = 0;
  char *p = line;
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

char **parse_line_arguments(char *line) {
  size_t args_num = count_args(line);

  if (args_num < 1) {
    return NULL;
  }

  char **args = malloc((args_num + 1) * sizeof(char *));
  char *p = line;

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
