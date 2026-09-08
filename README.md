# Command-Line Line Editor

## Team Overview
This project was designed and implemented as a compact systems-programming exercise with a focus on memory-safe C data structures, user-facing command parsing, and documentation quality.

- Systems Programming Lead: Responsible for data-structure design, file I/O, memory management, and command processing.
- Technical Writer: Responsible for usage documentation, examples, and project design notes.
- Quality Focus: Robust validation, clean compilation, and detailed help material.

## Build and Run
Compile the project with:

```bash
gcc main.c -o line_editor
```

Run it with:

```bash
./line_editor
```

You can also load a file at startup:

```bash
./line_editor sample.txt
```

## Feature Checklist
- [x] Insert text at a specific line number
- [x] Delete a line by index
- [x] Display all lines with numbering
- [x] Load a .txt file into memory
- [x] Save the in-memory document back to disk
- [x] Search for text and print matching lines
- [x] Find and replace text across the document
- [x] Show line, word, and character statistics
- [x] Validate negative or invalid indexes safely
- [x] Free allocated memory on exit and on deletion
- [x] Compile cleanly with gcc -Wall -Wextra -std=c11

## Data Structure Trade-off Justification
The editor stores each line in a doubly linked list of dynamic string nodes.

Advantages:
- Efficient insertion and deletion without shifting the entire document
- Easy to remove a line and reclaim memory immediately
- Very natural for editor-style operations where the document is frequently modified

Trade-offs:
- Each node stores both next and previous pointers, so memory overhead is higher than a plain contiguous array
- Sequential access is still O(n), which is acceptable for a terminal-based text editor with small to medium files

This design was chosen because editing commands are insertion and deletion heavy, and list-based operations preserve correctness and simplicity without the complexity of dynamic array reallocation logic.

## Notes
The command loop is interactive and validates bad input instead of crashing. Empty documents are handled gracefully, file operations are verified, and all heap memory is released before exit.
#
