# mysh — Minimal Unix Shell

`mysh` is a lightweight Unix shell implemented in C.
It provides an interactive command prompt with its **own readline implementation**, **command history**, and **inline line editing** (similar to GNU readline, but written from scratch).

This project is intended for learning how shells work under the hood — parsing input, handling terminal modes, forking, executing programs, and managing history.

---

## ✨ Features

| Feature                   | Description                                                     |
| ------------------------- | --------------------------------------------------------------- |
| **Custom readline**       | Raw mode input handling without using GNU `readline`            |
| **Editable command line** | Move cursor, insert text in the middle, delete characters, etc. |
| **Command history**       | Navigate previous commands using **↑** and **↓**                |
| **Cursor movement**       | `←`/`→`, `Ctrl+A`, `Ctrl+E`, word-based jumps, etc.             |
| **Command execution**     | Runs external programs using `fork()` + `execvp()`              |
| **Whitespace trimming**   | Input is stripped before execution                              |

---

## ⌨️ Keybindings

| Keys          | Action                            |
| ------------- | --------------------------------- |
| **← / →**     | Move cursor by one character      |
| **↑ / ↓**     | Browse history                    |
| **Ctrl+A**    | Move cursor to start of line      |
| **Ctrl+E**    | Move cursor to end of line        |
| **Ctrl+U**    | Clear text before cursor          |
| **Ctrl+K**    | Clear text after cursor           |
| **Ctrl+C**    | Cancel input and return to prompt |
| **Backspace** | Delete previous character         |

---

## Requirements

- A C compiler (e.g., `gcc` or `clang`)
- `make` utility

## How to run

```
make run
```
