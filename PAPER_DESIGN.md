# Paper Design Outline

## 1. Problem Summary
Build a command-line line editor that supports:
- insert
- delete
- display
- save
- load
- search
- statistics

The document must be stored in memory safely, support 1-based line indexing, and avoid memory leaks.

## 2. Core Data Structures
```c
struct LineNode {
    char *text;
    struct LineNode *prev;
    struct LineNode *next;
};

struct Document {
    LineNode *head;
    LineNode *tail;
    size_t count;
    char *current_file;
};
```

### Data Structure Justification
- A linked list supports efficient insert/delete in the middle of the document.
- No large block of memory must be shifted when lines are inserted or removed.
- Each node owns its line text allocation, making releases straightforward.

## 3. Function Signatures
```c
static void trim_newline(char *text);
static char *duplicate_string(const char *text);
static bool parse_index(const char *input, size_t *index);
static void free_document(Document *doc);
static void append_line(Document *doc, const char *text);
static bool insert_line_at(Document *doc, size_t index, const char *text);
static bool delete_line_at(Document *doc, size_t index);
static void print_document(const Document *doc);
static void print_search_results(const Document *doc, const char *query);
static void print_statistics(const Document *doc);
static bool load_file(Document *doc, const char *path);
static bool save_file(const Document *doc, const char *path);
static bool execute_command(Document *doc, char *input);
```

## 4. Algorithm for Insert
### Input
- Document pointer
- Target line number `n`
- String to insert

### Steps
1. Validate that the index is in the range `[1, count + 1]`.
2. Allocate a new node and duplicate the text.
3. If the document is empty, set the new node as head and tail.
4. If inserting at the beginning, link the new node before the current head.
5. If inserting in the middle, find the node at position `n` and splice the new node before it.
6. If inserting at the end, attach the new node to the tail.
7. Increment the document count.

Pseudo-logic:
```text
if n == 1:
    new.next = head
    head.prev = new
    head = new
else:
    traverse to node at index n
    new.prev = current.prev
    new.next = current
    current.prev = new
```

## 5. Algorithm for Delete
### Input
- Document pointer
- Target line number `n`

### Steps
1. Validate that `n` is between `1` and `count`.
2. Traverse to the target node.
3. Rewire neighbors:
   - `prev.next = next`
   - `next.prev = prev`
4. If deleting the first line, update `head`.
5. If deleting the last line, update `tail`.
6. Free the node's text memory and the node itself.
7. Decrement the document count.

Pseudo-logic:
```text
target = find node at index n
if target.prev != NULL:
    target.prev.next = target.next
else:
    head = target.next

if target.next != NULL:
    target.next.prev = target.prev
else:
    tail = target.prev

free(target.text)
free(target)
count--
```

## 6. ASCII Memory Layout
```text
Document
  +----------------------+
  | head ----------------> [Node 1] ---> [Node 2] ---> [Node 3] ---> NULL
  |                       |            |            |
  |                       v            v            v
  |                    text="A"     text="B"     text="C"
  |                    prev=NULL    prev=Node1  prev=Node2
  |                    next=Node2   next=Node3  next=NULL
  +----------------------+
  | tail ------------------^
  | count = 3
  +----------------------+
```

## 7. File I/O Strategy
- `load_file()` opens a text file and reads each line into a new linked-list node.
- `save_file()` writes each stored line back to disk, adding a newline between entries.
- Invalid file names or read/write failures are reported without crashing the program.

## 8. Error Handling Strategy
- Negative or non-numeric indexes are rejected.
- Indexes greater than the current document length are blocked.
- Empty input lines are ignored.
- Memory is released before program exit.
- Unsupported commands produce a clear message and remain in the loop.
