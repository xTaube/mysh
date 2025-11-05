#include "readline.h"
#include "attrs.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define READLINE_BUFFER_SIZE 4096
#define HISTORY_RECORDS_LIMIT 10
#define ESCAPE_CHAR '\x1b'
#define ARROW_KEY_UP 'A'
#define ARROW_KEY_DOWN 'B'
#define ARROW_KEY_LEFT 'D'
#define ARROW_KEY_RIGHT 'C'
#define CTRL_A '\x01'
#define CTRL_D '\x04'
#define CTRL_E '\x05'
#define CTRL_U '\x15'
#define CTRL_K '\x0b'
#define CTRL_C '\x03'
#define CTRL_L '\x0c'
#define BACKSPACE '\x7f'

// internal global buffer, technically thread unsafe but shells are rather not
// meant to be multithreaded
static char *_readline_buffer = NULL;
static size_t _readline_buffer_size = READLINE_BUFFER_SIZE;
static size_t _readline_buffer_len;
static size_t _readline_buffer_cursor;

static void allocate_readline_buffer() {
  if (_readline_buffer != NULL) {
    return;
  }

  _readline_buffer = malloc(_readline_buffer_size);
  if (!_readline_buffer) {
    perror("Readline buffer error.");
    exit(EXIT_FAILURE);
  }
}

static void move_cursor_left(size_t n) {
  for (size_t i = 0; i < n; i++) {
    write(STDOUT_FILENO, "\x1b[D", 3);
  }
}

static void move_cursor_right(size_t n) {
  for (size_t i = 0; i < n; i++) {
    write(STDOUT_FILENO, "\x1b[C", 3);
  }
}

static void move_cursor_to_previous_word() {
  if (_readline_buffer_cursor <= 0) {
    return;
  }

  size_t cursor_move = 1;

  while (_readline_buffer_cursor - cursor_move > 0) {
    if (isspace((unsigned char)(_readline_buffer[_readline_buffer_cursor -
                                                 cursor_move - 1])) &&
        !isspace((unsigned char)(_readline_buffer[_readline_buffer_cursor -
                                                  cursor_move])))
      break;
    cursor_move++;
  }

  move_cursor_left(cursor_move);
  _readline_buffer_cursor -= cursor_move;
}

static void move_cursor_to_next_word() {
  if (_readline_buffer_cursor >= _readline_buffer_len) {
    return;
  }

  size_t cursor_move = 1;
  while (_readline_buffer_cursor + cursor_move < _readline_buffer_len) {
    if (!isspace((unsigned char)(_readline_buffer[_readline_buffer_cursor +
                                                  cursor_move])) &&
        isspace((unsigned char)(_readline_buffer[_readline_buffer_cursor +
                                                 cursor_move - 1])))
      break;
    cursor_move++;
  }
  move_cursor_right(cursor_move);
  _readline_buffer_cursor += cursor_move;
}

static void write_char_to_output(char c) {
  if (!isprint(c))
    return;

  if (_readline_buffer_len >= _readline_buffer_size) {
    // (TODO): do re-size to handle input
    return;
  }

  if (_readline_buffer_cursor < _readline_buffer_len) {
    memmove(_readline_buffer + _readline_buffer_cursor + 1,
            _readline_buffer + _readline_buffer_cursor,
            _readline_buffer_len - _readline_buffer_cursor);
    _readline_buffer[_readline_buffer_cursor] = c;
    _readline_buffer_len++;
    _readline_buffer_cursor++;

    write(STDOUT_FILENO, _readline_buffer + _readline_buffer_cursor - 1,
          _readline_buffer_len - _readline_buffer_cursor + 1);

    move_cursor_left(_readline_buffer_len - _readline_buffer_cursor);
  } else {
    _readline_buffer[_readline_buffer_len] = c;
    _readline_buffer_len++;
    _readline_buffer_cursor++;
    write(STDOUT_FILENO, &c, 1);
  }
}

static void remove_char_from_output() {
  if (_readline_buffer_cursor <= 0)
    return;

  memmove(_readline_buffer + _readline_buffer_cursor - 1,
          _readline_buffer + _readline_buffer_cursor,
          _readline_buffer_len - _readline_buffer_cursor);
  _readline_buffer_len--;
  _readline_buffer_cursor--;
  write(STDOUT_FILENO, "\b", 1);
  write(STDOUT_FILENO, _readline_buffer + _readline_buffer_cursor,
        _readline_buffer_len - _readline_buffer_cursor);
  write(STDOUT_FILENO, " ", 1);
  move_cursor_left((_readline_buffer_len - _readline_buffer_cursor) + 1);
}

static void clear_line_before_cursor() {
  if (_readline_buffer_cursor <= 0)
    return;

  size_t prev_len = _readline_buffer_len;
  _readline_buffer_len -= _readline_buffer_cursor;

  memmove(_readline_buffer, _readline_buffer + _readline_buffer_cursor,
          prev_len - _readline_buffer_cursor);

  move_cursor_left(_readline_buffer_cursor);

  write(STDOUT_FILENO, _readline_buffer, prev_len - _readline_buffer_cursor);

  _readline_buffer_cursor = _readline_buffer_len;

  for (size_t i = 0; i < prev_len - _readline_buffer_cursor; i++)
    write(STDOUT_FILENO, " ", 1);

  move_cursor_left(prev_len);
  _readline_buffer_cursor = 0;
}

static void clear_word_before_cursor() {
  if (_readline_buffer_cursor <= 0)
    return;

  size_t prev_cursor = _readline_buffer_cursor;
  move_cursor_to_previous_word();

  size_t new_len =
      _readline_buffer_len - (prev_cursor - _readline_buffer_cursor);

  memmove(_readline_buffer + _readline_buffer_cursor,
          _readline_buffer + prev_cursor, _readline_buffer_len - prev_cursor);

  write(STDOUT_FILENO, _readline_buffer + _readline_buffer_cursor,
        _readline_buffer_len - prev_cursor);

  _readline_buffer_cursor += _readline_buffer_len - prev_cursor;

  for (size_t i = _readline_buffer_cursor; i < _readline_buffer_len; i++) {
    write(STDOUT_FILENO, " ", 1);
  }

  _readline_buffer_cursor -= _readline_buffer_len - prev_cursor;
  move_cursor_left(_readline_buffer_len - _readline_buffer_cursor);

  _readline_buffer_len = new_len;
}

static void clear_line_after_cursor() {
  if (_readline_buffer_cursor >= _readline_buffer_len)
    return;

  for (size_t i = 0; i < _readline_buffer_len - _readline_buffer_cursor; i++) {
    write(STDOUT_FILENO, " ", 1);
  }

  move_cursor_left(_readline_buffer_len - _readline_buffer_cursor);
  _readline_buffer_len = _readline_buffer_cursor;
}

static void enable_raw_mode() {
  struct termios raw;
  tcgetattr(STDIN_FILENO, &raw);
  raw.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static void disable_raw_mode() {
  struct termios raw;
  tcgetattr(STDIN_FILENO, &raw);
  raw.c_lflag &= (ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static size_t _history_size = HISTORY_RECORDS_LIMIT;
static size_t _history_buffer_index = 0;
static ssize_t _history_index = -1;
static char **__history = NULL;

static void allocate_history_buffer() {
  if (__history)
    return;

  __history = malloc(sizeof(char *) * _history_size);
  for (size_t i = 0; i < _history_size; i++) {
    __history[i] = NULL;
  }
}

void add_history(char *line) {
  if (strlen(line) == 0)
    return;

  if (__history[_history_buffer_index])
    free(__history[_history_buffer_index]);

  __history[_history_buffer_index] = strdup(line);
  _history_buffer_index = (_history_buffer_index + 1) % _history_size;
  _history_index = -1;
}

static void history_next() {
  if (_history_index == -1) {
    return;
  }
  if ((_history_index + 1) % _history_size != _history_buffer_index)
    _history_index = (_history_index + 1) % _history_size;

  if (!__history[_history_index]) {
    return;
  }

  move_cursor_left(_readline_buffer_cursor);

  size_t prev_len = _readline_buffer_len;
  _readline_buffer_len = strlen(__history[_history_index]);
  _readline_buffer_cursor = _readline_buffer_len;
  memcpy(_readline_buffer, __history[_history_index], _readline_buffer_len);
  write(STDOUT_FILENO, _readline_buffer, _readline_buffer_len);

  if (prev_len > _readline_buffer_len) {
    for (size_t i = 0; i < prev_len - _readline_buffer_len; i++)
      write(STDOUT_FILENO, " ", 1);

    move_cursor_left(prev_len - _readline_buffer_len);
  }
}

static void history_prev() {
  if (_history_index == -1) {
    _history_index = _history_buffer_index;
  }

  if (((_history_index - 1) >= 0 ? _history_index - 1 : _history_size - 1) !=
      _history_buffer_index) {
    ssize_t new_history_index =
        ((_history_index - 1) >= 0 ? _history_index - 1 : _history_size - 1);
    if (!__history[new_history_index])
      return;
    _history_index = new_history_index;
  }

  for (size_t i = 0; i < _readline_buffer_cursor; i++)
    write(STDOUT_FILENO, "\x1b[D", 3);

  size_t prev_len = _readline_buffer_len;
  _readline_buffer_len = strlen(__history[_history_index]);
  _readline_buffer_cursor = _readline_buffer_len;
  memcpy(_readline_buffer, __history[_history_index], _readline_buffer_len);
  write(STDOUT_FILENO, _readline_buffer, _readline_buffer_len);

  if (prev_len > _readline_buffer_len) {
    for (size_t i = 0; i < prev_len - _readline_buffer_len; i++)
      write(STDOUT_FILENO, " ", 1);
    move_cursor_left(prev_len - _readline_buffer_len);
  }
}

ATTR_NODISCARD ATTR_ALLOC char *readline(char *prompt) {
  allocate_readline_buffer();
  allocate_history_buffer();

  enable_raw_mode();
  write(STDOUT_FILENO, prompt, strlen(prompt));

  _readline_buffer_len = 0;
  _readline_buffer_cursor = 0;
  while (1) {
    char c;

    if (read(STDIN_FILENO, &c, 1) <= 0) {
      perror("Char read error");
      break;
    }
    if (c == '\n') {
      write(STDOUT_FILENO, "\n", 1);
      _readline_buffer[_readline_buffer_len] = '\0';
      break;
    } else if (c == CTRL_D) {
      if (_readline_buffer_len == 0) {
        write(STDOUT_FILENO, "^D\n", 3);
        disable_raw_mode();
        return NULL;
      }
      break;
    } else if (c == BACKSPACE) { // backspace
      remove_char_from_output();
    } else if (c == ESCAPE_CHAR) {
      char seq[2] = {0};
      ssize_t nread = read(STDIN_FILENO, &seq, sizeof(seq));
      if (nread < 1)
        continue;
      if (seq[0] == '[') {
        if (nread < 2)
          continue;
        if (seq[1] == ARROW_KEY_LEFT && _readline_buffer_cursor > 0) {
          move_cursor_left(1);
          _readline_buffer_cursor--;
        } else if (seq[1] == ARROW_KEY_RIGHT &&
                   _readline_buffer_cursor < _readline_buffer_len) {
          move_cursor_right(1);
          _readline_buffer_cursor++;
        } else if (seq[1] == ARROW_KEY_UP) {
          history_prev();
        } else if (seq[1] == ARROW_KEY_DOWN) {
          history_next();
        }
      } else if (seq[0] == 'b') { // move cursor to previous word
        move_cursor_to_previous_word();
      } else if (seq[0] == 'f') { // move cursor to next word
        move_cursor_to_next_word();
      } else if (seq[0] == BACKSPACE) { // clear word before cursor
        clear_word_before_cursor();
      }
    } else if (c == CTRL_A) { // move to start of line
      move_cursor_left(_readline_buffer_cursor);
      _readline_buffer_cursor = 0;
    } else if (c == CTRL_E) { // move to end of line
      move_cursor_right(_readline_buffer_len - _readline_buffer_cursor);
      _readline_buffer_cursor = _readline_buffer_len;
    } else if (c == CTRL_U) { // clear line before cursor
      clear_line_before_cursor();
    } else if (c == CTRL_K) {
      clear_line_after_cursor();
    } else if (c == CTRL_C) {
      write(STDOUT_FILENO, "^C\n", 3);
      disable_raw_mode();
      return NULL;
    } else {
      write_char_to_output(c);
    }
  }

  disable_raw_mode();

  // strip right
  char *p = _readline_buffer + _readline_buffer_len - 1;
  while (p >= _readline_buffer && isspace((unsigned char)*p))
    p--;
  *(p + 1) = '\0';

  // strip left
  p = _readline_buffer;
  while (isspace((unsigned char)*p))
    p++;
  memmove(_readline_buffer, p, strlen(p) + 1);

  char *line = strdup(_readline_buffer);
  if (!line) {
    perror("strdup error.");
    return NULL;
  }
  return line;
}
