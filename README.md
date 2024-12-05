# Text-processing-utilities

Handful of fun utilities for text processing

## Building

This project uses [xmake](https://xmake.io) and uses [clang](https://clang.llvm.org/) specific C features. Once that is installed, build with:

```sh
xmake config --mode=release --toolchain=clang
xmake
```

an executable will be present in `build/<OS>/<ARCH>/[debug|release]/text-editor`


## Usage

```sh
./main <command> <param1> <param2> ...
```

### Example:

```sh
./main help
./main create-file test.txt
./main copy-file test.txt test2.txt
./main show-line test.txt 1
./main find test.txt "search string"
./main trim test.txt
./main show-change-log
```

## Current list of commands:

- `create-file <filename>` - Creates a file
- `copy-file <source> <destination>` - Copies a file to a destination
- `delete-file <filename>` removes a file
- `show-file <filename>` shows the contents of a file
- `append-line <filename> <content>` appends data to a file
- `delete-line <filename> <line number>` deletes a specific line from a file
- `insert-line <filename> <line number> <data>` inserts a line into a file, shifting all data after it downwards
- `show-line <filename> <line number>` shows the content of a specific line
- `changelog`
- `line-count`
- `trim`
- `find`
- `help`

