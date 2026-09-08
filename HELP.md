# Line Editor User Manual

## Overview
This editor works in an interactive terminal. Each line is stored as a separate document entry, and commands are entered at the `editor>` prompt.

## Command Syntax

### 1) Help
```bash
help
```
Prints a list of all supported commands.

### 2) Display Document
```bash
display
```
Prints all lines numbered consecutively in the format:

```text
1 | First line
2 | Second line
```

### 3) Insert Text
```bash
insert <line_number> <text>
```
Examples:

```bash
insert 1 Hello world
insert 3 This is a new line
insert 10 Final line
```

Behavior:
- Uses 1-based indexing.
- `insert 1 ...` adds before the first line.
- `insert N ...` where `N = current_line_count + 1` appends to the end.
- Existing lines shift downward.

### 4) Delete a Line
```bash
delete <line_number>
```
Examples:

```bash
delete 1
delete 5
```

Behavior:
- Deletes the chosen line.
- Later lines shift upward.
- Out-of-range indexes are rejected with a clear message.

### 5) Load Document from a File
```bash
load <file.txt>
```
Example:

```bash
load sample.txt
```

Behavior:
- Opens the selected text file.
- Clears the current in-memory document.
- Loads all lines into the editor.

### 6) Save Document to a File
```bash
save [file.txt]
```
Examples:

```bash
save notes.txt
save
```

Behavior:
- If a filename is provided, the document is saved there.
- If no filename is given and the document was loaded from a file, it saves back to the original file.

### 7) Search Text
```bash
search <text>
```
Example:

```bash
search error
```

Behavior:
- Prints every line containing the search string.
- Shows matching line numbers and content.

### 7) Find Text
```bash
find <text>
```
Example:

```bash
find error
```

This behaves like a quick search and prints any matching lines.

### 8) Replace Text
```bash
replace <old_text> <new_text>
```
Example:

```bash
replace hello world
```

Behavior:
- Replaces every matching occurrence of the old text in the document.
- Works across all lines.
- Prints a confirmation when a change is made.

### 9) Statistics
```bash
stats
```
Shows:
- Total number of lines
- Total number of words
- Total number of characters

### 10) Quit
```bash
quit
```
Ends the editor session and frees all memory.

## Example Session
```bash
./line_editor sample.txt
editor> display
1 | hello
2 | world
editor> insert 2 new line
Inserted at line 2.
editor> find world
2 | world
editor> replace hello hi
Replacement complete.
editor> stats
Line count: 3
Word count: 5
Character count: 20
editor> save sample.txt
Document saved to 'sample.txt'.
editor> quit
```

## Validation Rules
The program rejects invalid input like:
- letters instead of numbers
- negative indexes
- zero indexes
- index values larger than the current document length

These are handled cleanly with error messages instead of crashes or hanging loops.
