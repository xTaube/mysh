#ifndef PARSERS_H
#define PARSERS_H

#include "attrs.h"
#include <ctype.h>

size_t parse_line_commands(char *line, char **commands[]);
char **parse_command_arguments(char *command);

#endif
