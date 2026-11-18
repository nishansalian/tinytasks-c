#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_FILE ".todo_tasks"
#define MAX_LINE 4096

typedef struct {
    unsigned long id;
    int completed;
    char *description;
} Task;

typedef struct {
    Task *items;
    size_t count;
    size_t capacity;
} TaskList;

static void free_tasks(TaskList *list) {
    for (size_t i = 0; i < list->count; ++i) free(list->items[i].description);
    free(list->items);
    list->items = NULL;
    list->count = list->capacity = 0;
}

static int append_task(TaskList *list, unsigned long id, int completed, const char *description) {
    if (list->count == list->capacity) {
        size_t next = list->capacity ? list->capacity * 2 : 8;
        Task *items = realloc(list->items, next * sizeof(*items));
        if (!items) return 0;
        list->items = items;
        list->capacity = next;
    }
    char *copy = strdup(description);
    if (!copy) return 0;
    list->items[list->count++] = (Task){id, completed, copy};
    return 1;
}

static int load_tasks(TaskList *list) {
    FILE *file = fopen(DATA_FILE, "r");
    if (!file) return errno == ENOENT;

    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;
    while ((length = getline(&line, &capacity, file)) != -1) {
        if (length && line[length - 1] == '\n') line[--length] = '\0';
        char *first = strchr(line, '|');
        char *second = first ? strchr(first + 1, '|') : NULL;
        if (!first || !second) continue;
        *first = '\0';
        *second = '\0';
        char *end = NULL;
        errno = 0;
        unsigned long id = strtoul(line, &end, 10);
        if (errno || end == line || *end || !id) continue;
        if (!append_task(list, id, first[1] == '1', second + 1)) {
            free(line);
            fclose(file);
            return 0;
        }
    }
    free(line);
    int ok = !ferror(file);
    fclose(file);
    return ok;
}

static int save_tasks(const TaskList *list) {
    FILE *file = fopen(DATA_FILE, "w");
    if (!file) {
        perror("todo: cannot save tasks");
        return 0;
    }
    for (size_t i = 0; i < list->count; ++i) {
        if (fprintf(file, "%lu|%d|%s\n", list->items[i].id,
                    list->items[i].completed, list->items[i].description) < 0) {
            perror("todo: cannot save tasks");
            fclose(file);
            return 0;
        }
    }
    if (fclose(file) != 0) {
        perror("todo: cannot save tasks");
        return 0;
    }
    return 1;
}

static void print_usage(FILE *out) {
    fprintf(out,
        "Usage:\n"
        "  todo add <task description>\n"
        "  todo list\n"
        "  todo done <id>\n"
        "  todo delete <id>\n"
        "  todo help\n\n"
        "Tasks are saved in %s in the current directory.\n", DATA_FILE);
}

static char *join_args(int argc, char **argv, int start) {
    size_t length = 1;
    for (int i = start; i < argc; ++i) length += strlen(argv[i]) + 1;
    char *result = malloc(length);
    if (!result) return NULL;
    result[0] = '\0';
    for (int i = start; i < argc; ++i) {
        if (i > start) strcat(result, " ");
        strcat(result, argv[i]);
    }
    return result;
}

static int parse_id(const char *text, unsigned long *id) {
    char *end = NULL;
    errno = 0;
    unsigned long value = strtoul(text, &end, 10);
    if (errno || end == text || *end || value == 0) return 0;
    *id = value;
    return 1;
}

static int run_command(TaskList *list, int argc, char **argv) {
    if (argc < 2 || strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0) {
        print_usage(stdout);
        return 0;
    }
    const char *command = argv[1];
    if (strcmp(command, "add") == 0) {
        if (argc < 3) {
            fprintf(stderr, "todo: add needs a task description\n");
            return 2;
        }
        char *description = join_args(argc, argv, 2);
        if (!description) {
            perror("todo: out of memory");
            return 1;
        }
        if (strchr(description, '|') || strpbrk(description, "\r\n")) {
            fprintf(stderr, "todo: task descriptions cannot contain '|' or line breaks\n");
            free(description);
            return 2;
        }
        unsigned long id = 1;
        for (size_t i = 0; i < list->count; ++i)
            if (list->items[i].id >= id && list->items[i].id < ULONG_MAX)
                id = list->items[i].id + 1;
        if (!append_task(list, id, 0, description)) {
            free(description);
            perror("todo: out of memory");
            return 1;
        }
        printf("Added #%lu: %s\n", id, description);
        free(description);
        return save_tasks(list) ? 0 : 1;
    }
    if (strcmp(command, "list") == 0) {
        if (!list->count) {
            puts("No tasks yet. Add one with: todo add <description>");
            return 0;
        }
        for (size_t i = 0; i < list->count; ++i)
            printf("[%c] #%lu %s\n", list->items[i].completed ? 'x' : ' ',
                   list->items[i].id, list->items[i].description);
        return 0;
    }
    if (strcmp(command, "done") == 0 || strcmp(command, "delete") == 0) {
        unsigned long id;
        if (argc != 3 || !parse_id(argv[2], &id)) {
            fprintf(stderr, "todo: %s needs a valid task ID\n", command);
            return 2;
        }
        for (size_t i = 0; i < list->count; ++i) {
            if (list->items[i].id != id) continue;
            if (strcmp(command, "done") == 0) {
                if (list->items[i].completed) {
                    printf("Task #%lu is already complete.\n", id);
                    return 0;
                }
                list->items[i].completed = 1;
                printf("Completed #%lu: %s\n", id, list->items[i].description);
            } else {
                printf("Deleted #%lu: %s\n", id, list->items[i].description);
                free(list->items[i].description);
                memmove(&list->items[i], &list->items[i + 1],
                        (list->count - i - 1) * sizeof(*list->items));
                --list->count;
            }
            return save_tasks(list) ? 0 : 1;
        }
        fprintf(stderr, "todo: no task with ID %lu\n", id);
        return 1;
    }
    fprintf(stderr, "todo: unknown command '%s'\n", command);
    print_usage(stderr);
    return 2;
}

int main(int argc, char **argv) {
    TaskList list = {0};
    if (!load_tasks(&list)) {
        fprintf(stderr, "todo: could not read %s\n", DATA_FILE);
        free_tasks(&list);
        return 1;
    }
    int result = run_command(&list, argc, argv);
    free_tasks(&list);
    return result;
}
