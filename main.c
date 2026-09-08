#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INPUT_BUFFER_SIZE 4096

/*
 * Data Structure Choice:
 * We use a doubly linked list of LineNode structures.
 *
 * Why this structure?
 * - Insertion and deletion by line number are efficient when we traverse to the
 *   target node: O(n) time because we only move pointers, without copying or
 *   shifting the entire array.
 * - Deleting or inserting a line at the middle does not require relocating all
 *   subsequent strings in contiguous memory.
 * - It also makes memory cleanup straightforward: each node owns one heap
 *   allocation for its line text, and free_document() traverses the chain and
 *   releases every allocation.
 *
 * Trade-off:
 * - Each node has a pointer to the previous and next node, so the memory
 *   overhead per line is higher than a compact array representation.
 * - A dynamic array may have better cache locality and slightly lower constant
 *   factors for append-only workloads, but linked lists are more natural for
 *   arbitrary insertion and deletion in a text editor.
 */
typedef struct LineNode {
    char *text;
    struct LineNode *prev;
    struct LineNode *next;
} LineNode;

typedef struct {
    LineNode *head;
    LineNode *tail;
    size_t count;
    char *current_file;
} Document;

static void trim_newline(char *text);
static char *duplicate_string(const char *text);
static char *skip_spaces(char *text);
static bool parse_index(const char *input, size_t *index);
static void free_document(Document *doc);
static void append_line(Document *doc, const char *text);
static bool insert_line_at(Document *doc, size_t index, const char *text);
static bool delete_line_at(Document *doc, size_t index);
static void print_document(const Document *doc);
static void print_search_results(const Document *doc, const char *query);
static char *replace_all_in_string(const char *source, const char *old_text, const char *new_text);
static bool replace_all_in_document(Document *doc, const char *old_text, const char *new_text);
static void print_statistics(const Document *doc);
static bool load_file(Document *doc, const char *path);
static bool save_file(const Document *doc, const char *path);
static void print_usage(void);
static void print_help(void);
static bool execute_command(Document *doc, char *input);

int main(int argc, char **argv) {
    Document doc = {NULL, NULL, 0, NULL};

    if (argc > 2) {
        print_usage();
        return EXIT_FAILURE;
    }

    if (argc == 2) {
        if (!load_file(&doc, argv[1])) {
            fprintf(stderr, "Error: could not load file '%s'.\n", argv[1]);
            free_document(&doc);
            return EXIT_FAILURE;
        }
    }

    printf("Command-Line Line Editor\n");
    printf("Type 'help' to see available commands.\n");

    char input[INPUT_BUFFER_SIZE];
    while (1) {
        printf("editor> ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\nGoodbye.\n");
            break;
        }

        trim_newline(input);

        if (input[0] == '\0') {
            continue;
        }

        if (!execute_command(&doc, input)) {
            break;
        }
    }

    free_document(&doc);
    return EXIT_SUCCESS;
}

static void trim_newline(char *text) {
    if (text == NULL) {
        return;
    }

    size_t length = strlen(text);
    while (length > 0 && (text[length - 1] == '\n' || text[length - 1] == '\r')) {
        text[length - 1] = '\0';
        length--;
    }
}

static char *duplicate_string(const char *text) {
    if (text == NULL) {
        return NULL;
    }

    size_t length = strlen(text);
    char *copy = malloc(length + 1U);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, text, length + 1U);
    return copy;
}

static char *skip_spaces(char *text) {
    if (text == NULL) {
        return NULL;
    }

    while (*text != '\0' && isspace((unsigned char)*text)) {
        text++;
    }

    return text;
}

static bool parse_index(const char *input, size_t *index) {
    char *end_ptr = NULL;
    unsigned long long value;

    if (input == NULL || index == NULL || input[0] == '\0') {
        return false;
    }

    if (input[0] == '-' || input[0] == '+') {
        return false;
    }

    errno = 0;
    value = strtoull(input, &end_ptr, 10);
    if (errno == ERANGE || end_ptr == input || *end_ptr != '\0') {
        return false;
    }

    if (value > SIZE_MAX) {
        return false;
    }

    *index = (size_t)value;
    return true;
}

static void free_document(Document *doc) {
    if (doc == NULL) {
        return;
    }

    LineNode *current = doc->head;
    while (current != NULL) {
        LineNode *next = current->next;
        free(current->text);
        free(current);
        current = next;
    }

    free(doc->current_file);
    doc->head = NULL;
    doc->tail = NULL;
    doc->count = 0;
    doc->current_file = NULL;
}

static void append_line(Document *doc, const char *text) {
    LineNode *node = malloc(sizeof(*node));
    if (node == NULL) {
        fprintf(stderr, "Error: out of memory while creating a new line.\n");
        exit(EXIT_FAILURE);
    }

    node->text = duplicate_string(text);
    if (node->text == NULL) {
        free(node);
        fprintf(stderr, "Error: out of memory while duplicating line text.\n");
        exit(EXIT_FAILURE);
    }

    node->prev = doc->tail;
    node->next = NULL;

    if (doc->head == NULL) {
        doc->head = node;
        doc->tail = node;
    } else {
        doc->tail->next = node;
        doc->tail = node;
    }

    doc->count++;
}

static bool insert_line_at(Document *doc, size_t index, const char *text) {
    LineNode *new_node;
    LineNode *current;

    if (doc == NULL || text == NULL) {
        return false;
    }

    if (index == 0 || index > doc->count + 1U) {
        return false;
    }

    new_node = malloc(sizeof(*new_node));
    if (new_node == NULL) {
        fprintf(stderr, "Error: out of memory while inserting a line.\n");
        return false;
    }

    new_node->text = duplicate_string(text);
    if (new_node->text == NULL) {
        free(new_node);
        fprintf(stderr, "Error: out of memory while duplicating line text.\n");
        return false;
    }

    new_node->prev = NULL;
    new_node->next = NULL;

    if (doc->count == 0) {
        doc->head = new_node;
        doc->tail = new_node;
        doc->count = 1U;
        return true;
    }

    if (index == 1U) {
        new_node->next = doc->head;
        doc->head->prev = new_node;
        doc->head = new_node;
        doc->count++;
        return true;
    }

    current = doc->head;
    size_t position = 1U;
    while (current != NULL && position < index) {
        current = current->next;
        position++;
    }

    if (current == NULL) {
        /* Inserting at the end, after the last line. */
        new_node->prev = doc->tail;
        doc->tail->next = new_node;
        doc->tail = new_node;
        doc->count++;
        return true;
    }

    new_node->prev = current->prev;
    new_node->next = current;

    if (current->prev != NULL) {
        current->prev->next = new_node;
    } else {
        doc->head = new_node;
    }

    current->prev = new_node;
    doc->count++;
    return true;
}

static bool delete_line_at(Document *doc, size_t index) {
    LineNode *target;
    size_t position;

    if (doc == NULL || doc->count == 0 || index == 0 || index > doc->count) {
        return false;
    }

    target = doc->head;
    position = 1U;
    while (target != NULL && position < index) {
        target = target->next;
        position++;
    }

    if (target == NULL) {
        return false;
    }

    if (target->prev != NULL) {
        target->prev->next = target->next;
    } else {
        doc->head = target->next;
    }

    if (target->next != NULL) {
        target->next->prev = target->prev;
    } else {
        doc->tail = target->prev;
    }

    free(target->text);
    free(target);
    doc->count--;

    if (doc->count == 0U) {
        doc->head = NULL;
        doc->tail = NULL;
    }

    return true;
}

static void print_document(const Document *doc) {
    LineNode *current;
    size_t line_number = 1U;

    if (doc == NULL) {
        return;
    }

    if (doc->count == 0U) {
        printf("No lines in the document.\n");
        return;
    }

    current = doc->head;
    while (current != NULL) {
        printf("%zu | %s\n", line_number, current->text);
        current = current->next;
        line_number++;
    }
}

static void print_search_results(const Document *doc, const char *query) {
    LineNode *current;
    size_t line_number = 1U;
    bool found = false;

    if (doc == NULL || query == NULL || query[0] == '\0') {
        printf("Search query cannot be empty.\n");
        return;
    }

    current = doc->head;
    while (current != NULL) {
        if (strstr(current->text, query) != NULL) {
            printf("%zu | %s\n", line_number, current->text);
            found = true;
        }
        current = current->next;
        line_number++;
    }

    if (!found) {
        printf("No matching lines found for: %s\n", query);
    }
}

static char *replace_all_in_string(const char *source, const char *old_text, const char *new_text) {
    const char *cursor;
    const char *match;
    char *result;
    char *write;
    size_t count = 0U;
    size_t old_len;
    size_t new_len;
    size_t source_len;
    size_t final_len;

    if (source == NULL || old_text == NULL || new_text == NULL || old_text[0] == '\0') {
        return NULL;
    }

    old_len = strlen(old_text);
    new_len = strlen(new_text);
    source_len = strlen(source);

    cursor = source;
    while ((match = strstr(cursor, old_text)) != NULL) {
        count++;
        cursor = match + old_len;
    }

    final_len = source_len + count * (new_len - old_len) + 1U;
    result = malloc(final_len);
    if (result == NULL) {
        return NULL;
    }

    write = result;
    cursor = source;
    while ((match = strstr(cursor, old_text)) != NULL) {
        size_t prefix_len = (size_t)(match - cursor);
        memcpy(write, cursor, prefix_len);
        write += prefix_len;
        memcpy(write, new_text, new_len);
        write += new_len;
        cursor = match + old_len;
    }

    strcpy(write, cursor);
    return result;
}

static bool replace_all_in_document(Document *doc, const char *old_text, const char *new_text) {
    LineNode *current;
    bool replaced = false;

    if (doc == NULL || old_text == NULL || new_text == NULL || old_text[0] == '\0') {
        return false;
    }

    current = doc->head;
    while (current != NULL) {
        char *updated = replace_all_in_string(current->text, old_text, new_text);
        if (updated == NULL) {
            return false;
        }

        if (strcmp(updated, current->text) != 0) {
            replaced = true;
        }

        free(current->text);
        current->text = updated;
        current = current->next;
    }

    return replaced;
}

static void print_statistics(const Document *doc) {
    LineNode *current;
    size_t total_words = 0U;
    size_t total_characters = 0U;
    bool in_word = false;

    if (doc == NULL) {
        return;
    }

    current = doc->head;
    while (current != NULL) {
        size_t i = 0U;
        while (current->text[i] != '\0') {
            total_characters++;
            if (isspace((unsigned char)current->text[i])) {
                if (in_word) {
                    total_words++;
                    in_word = false;
                }
            } else {
                in_word = true;
            }
            i++;
        }
        if (in_word) {
            total_words++;
            in_word = false;
        }
        current = current->next;
    }

    printf("Line count: %zu\n", doc->count);
    printf("Word count: %zu\n", total_words);
    printf("Character count: %zu\n", total_characters);
}

static bool load_file(Document *doc, const char *path) {
    FILE *file;
    char buffer[INPUT_BUFFER_SIZE];

    if (doc == NULL || path == NULL || path[0] == '\0') {
        return false;
    }

    file = fopen(path, "r");
    if (file == NULL) {
        return false;
    }

    free_document(doc);

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        trim_newline(buffer);
        append_line(doc, buffer);
    }

    fclose(file);

    free(doc->current_file);
    doc->current_file = duplicate_string(path);
    if (doc->current_file == NULL) {
        fprintf(stderr, "Warning: unable to retain current file path.\n");
    }

    return true;
}

static bool save_file(const Document *doc, const char *path) {
    FILE *file;
    LineNode *current;

    if (doc == NULL || path == NULL || path[0] == '\0') {
        return false;
    }

    file = fopen(path, "w");
    if (file == NULL) {
        return false;
    }

    current = doc->head;
    while (current != NULL) {
        if (current->text != NULL) {
            fputs(current->text, file);
            fputc('\n', file);
        }
        current = current->next;
    }

    fclose(file);
    return true;
}

static void print_usage(void) {
    printf("Usage: ./line_editor [file.txt]\n");
}

static void print_help(void) {
    printf("Available commands:\n");
    printf("  help\n");
    printf("  display\n");
    printf("  insert <line_number> <text>\n");
    printf("  delete <line_number>\n");
    printf("  load <file.txt>\n");
    printf("  save [file.txt]\n");
    printf("  search <text>\n");
    printf("  find <text>\n");
    printf("  replace <old_text> <new_text>\n");
    printf("  stats\n");
    printf("  quit\n");
    printf("\nNotes:\n");
    printf("  - Line numbers are 1-based.\n");
    printf("  - Insert at line 1 places the new text before the first line.\n");
    printf("  - Insert at line N where N = count + 1 appends at the end.\n");
    printf("  - replace updates every matching occurrence in the whole document.\n");
}

static bool execute_command(Document *doc, char *input) {
    char *command;
    char *rest;
    char *index_text;
    char *text_start;
    size_t index;

    if (doc == NULL || input == NULL) {
        return false;
    }

    command = skip_spaces(input);
    if (command[0] == '\0') {
        return true;
    }

    rest = command;
    while (*rest != '\0' && !isspace((unsigned char)*rest)) {
        rest++;
    }

    if (*rest != '\0') {
        *rest = '\0';
        rest = skip_spaces(rest + 1);
    } else {
        rest = "";
    }

    if (strcmp(command, "quit") == 0 || strcmp(command, "exit") == 0) {
        printf("Exiting editor.\n");
        return false;
    }

    if (strcmp(command, "help") == 0) {
        print_help();
        return true;
    }

    if (strcmp(command, "display") == 0 || strcmp(command, "list") == 0 || strcmp(command, "show") == 0) {
        print_document(doc);
        return true;
    }

    if (strcmp(command, "stats") == 0) {
        print_statistics(doc);
        return true;
    }

    if (strcmp(command, "load") == 0) {
        if (rest[0] == '\0') {
            printf("Usage: load <file.txt>\n");
            return true;
        }
        if (!load_file(doc, rest)) {
            fprintf(stderr, "Error: failed to load '%s'.\n", rest);
        }
        return true;
    }

    if (strcmp(command, "save") == 0) {
        char *save_path = rest;
        if (save_path[0] == '\0') {
            if (doc->current_file == NULL) {
                printf("Usage: save <file.txt>\n");
                return true;
            }
            save_path = doc->current_file;
        }
        if (!save_file(doc, save_path)) {
            fprintf(stderr, "Error: failed to save to '%s'.\n", save_path);
        } else {
            printf("Document saved to '%s'.\n", save_path);
        }
        return true;
    }

    if (strcmp(command, "search") == 0 || strcmp(command, "find") == 0) {
        if (rest[0] == '\0') {
            printf("Usage: search <text>\n");
            return true;
        }
        print_search_results(doc, rest);
        return true;
    }

    if (strcmp(command, "replace") == 0) {
        char *old_value;
        char *new_value;

        if (rest[0] == '\0') {
            printf("Usage: replace <old_text> <new_text>\n");
            return true;
        }

        old_value = rest;
        while (*old_value != '\0' && !isspace((unsigned char)*old_value)) {
            old_value++;
        }

        if (*old_value == '\0') {
            printf("Usage: replace <old_text> <new_text>\n");
            return true;
        }

        *old_value = '\0';
        new_value = skip_spaces(old_value + 1);
        if (new_value[0] == '\0') {
            printf("Usage: replace <old_text> <new_text>\n");
            return true;
        }

        if (!replace_all_in_document(doc, rest, new_value)) {
            printf("No replacements made.\n");
        } else {
            printf("Replacement complete.\n");
        }
        return true;
    }

    if (strcmp(command, "delete") == 0) {
        if (rest[0] == '\0') {
            printf("Usage: delete <line_number>\n");
            return true;
        }

        index_text = rest;
        while (*index_text != '\0' && !isspace((unsigned char)*index_text)) {
            index_text++;
        }

        if (*index_text != '\0') {
            *index_text = '\0';
            text_start = skip_spaces(index_text + 1);
            if (text_start[0] != '\0') {
                printf("Ignoring extra text after delete index.\n");
            }
        }

        if (!parse_index(rest, &index)) {
            printf("Invalid index. Please enter a positive integer.\n");
            return true;
        }

        if (!delete_line_at(doc, index)) {
            printf("Error: cannot delete line %zu. Index out of range.\n", index);
        } else {
            printf("Deleted line %zu.\n", index);
        }
        return true;
    }

    if (strcmp(command, "insert") == 0) {
        char *second_token;
        char *text;

        if (rest[0] == '\0') {
            printf("Usage: insert <line_number> <text>\n");
            return true;
        }

        index_text = rest;
        while (*index_text != '\0' && !isspace((unsigned char)*index_text)) {
            index_text++;
        }

        if (*index_text == '\0') {
            printf("Usage: insert <line_number> <text>\n");
            return true;
        }

        *index_text = '\0';
        second_token = skip_spaces(index_text + 1);
        if (second_token[0] == '\0') {
            printf("Usage: insert <line_number> <text>\n");
            return true;
        }

        if (!parse_index(rest, &index)) {
            printf("Invalid index. Please enter a positive integer.\n");
            return true;
        }

        text = second_token;
        if (!insert_line_at(doc, index, text)) {
            printf("Error: cannot insert at line %zu. Index out of range.\n", index);
        } else {
            printf("Inserted at line %zu.\n", index);
        }
        return true;
    }

    printf("Unknown command: %s\n", command);
    printf("Type 'help' for command usage.\n");
    return true;
}
