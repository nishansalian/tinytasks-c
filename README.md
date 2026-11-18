# TinyTasks

A small command-line to-do list written in C. Tasks are stored in a plain text file in the directory where you run the program, so they remain available the next time you use it.

## Build

You need a C compiler such as GCC or Clang and Make.

```sh
make
```

This creates the `todo` program in the project directory. You can also compile it directly:

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -O2 -o todo todo.c
```

## Use

```sh
./todo add Finish my C project
./todo list
./todo done 1
./todo delete 1
./todo help
```

Quote a task if you want to preserve punctuation or shell characters:

```sh
./todo add "Read chapter 3; take notes"
```

Tasks are saved in `.todo_tasks` in the current working directory. The file is created automatically when you add, complete, or delete a task. Task IDs stay stable when other tasks are deleted.

## Commands

| Command | What it does |
| --- | --- |
| `add <description>` | Save a new task |
| `list` | Show all tasks and their completion state |
| `done <id>` | Mark a task complete |
| `delete <id>` | Remove a task |
| `help` | Show command usage |

## License

MIT. See [LICENSE](LICENSE).
