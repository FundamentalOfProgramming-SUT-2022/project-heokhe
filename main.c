#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "utils.c"

#define ROOT "root"

int create_file(char* address) {
  int len = strlen(address);

  char* current_address = malloc(sizeof(char) * len);
  char* segment = malloc(sizeof(char) * 100);
  int segment_index = -1;

  for (int i = 0; i < len; i++) {
    char current = *(address + i);
    if (!current) break;

    if (i == 0 && current != '/') return 2;

    if (current == '/') {
      if (segment_index == 0 && !is_equal(segment, ROOT)) {
        return 1;
      }

      if (segment_index > 0) {
        strncat(current_address, "/", 1);
      }

      strncat(current_address, segment, strlen(segment));
      struct stat st = { 0 };
      if (!_exists(address)) {
        mkdir(current_address, 0700);
      }

      strcpy(segment, "");
      segment_index++;
    }
    else {
      strncat(segment, &current, 1);
    }
  }

  strncat(current_address, "/", 1);
  strncat(current_address, segment, strlen(segment));

  // make the actual file (the final touch 😁)
  _touch(current_address);

  return 0;
}

char* cat(char* address) {
  FILE* file = fopen(remove_leading_slash(address), "r");

  char* contents = malloc(sizeof(char) * 1e6);
  char ch;
  while ((ch = fgetc(file)) != EOF) {
    strncat(contents, &ch, 1);
  }

  fclose(file);

  return contents;
}

void write_with_history(char* address, char* content) {
  write_to_file(get_prev_version_name(address), cat(address));
  write_to_file(address, content);
}

void undo(char* address) {
  write_with_history(address, cat(get_prev_version_name(address)));
}

void insert(char* address, int line, int col, char* thing) {
  // lines start from 1, but we don't want that
  line--;

  char* new_contents = malloc(sizeof(char) * 1e4);
  FILE* file = fopen(remove_leading_slash(address), "r+");

  // every line before the wanted line
  for (int i = 0; i < line; i++) {
    for (;;) {
      char ch = fgetc(file);
      strncat(new_contents, &ch, 1);
      if (ch == '\n') break;
    }
  }

  // at the wanted line, befoe the wanted column
  for (int i = 0; i < col; i++) {
    char ch = fgetc(file);
    strncat(new_contents, &ch, 1);
  }

  // write the new stuff
  thing = convert_backspaces(thing);
  strncat(new_contents, thing, strlen(thing));

  // and finally, anything that comes after the wanted position
  char ch;
  while ((ch = fgetc(file)) != EOF) {
    strncat(new_contents, &ch, 1);
  }

  // end the read operation
  fclose(file);

  // write to the file
  write_with_history(address, new_contents);
}

void paste(char* address, int line, int col) {
  return insert(address, line, col, read_clipboard());
}

void removestr(char* address, int line, int col, int size, bool backward) {
  // For the love of all that's holy
  line--;

  char* contents = cat(address);
  int index = get_index_of_pos(contents, line, col);

  for (int i = 0; i < size; i++) {
    if (backward) {
      memmove(&contents[index - size + 1], &contents[index - size + 2], strlen(contents) - index + size - 1);
    }
    else {
      memmove(&contents[index], &contents[index + 1], strlen(contents) - index);
    }
  }

  write_with_history(address, contents);
}

void copy(char* address, int line, int col, int size, bool backward) {
  line--;

  char* contents = cat(address);
  char selected[size];
  int index = get_index_of_pos(contents, line, col);
  if (backward) {
    index -= size;
  }

  for (int i = 0; i < size; i++)
    selected[i] = contents[i + index];

  write_to_clipboard(selected);
}

void cut(char* address, int line, int col, int size, bool backward) {
  copy(address, line, col, size, backward);
  removestr(address, line, col, size, backward);
}

int main(int argc, char* argv[]) {
  char* command = argv[1];
  if (is_equal(command, "createfile")) {
    if (!is_equal(argv[2], "--file")) {
      printf("invalid format");
      return 0;
    }
    char* address = argv[3];
    int status = create_file(address);
    if (!status) {
      printf("Created %s \n", address);
    }
    else {
      // TODO: give an actual explanation
      printf("Failed to create %s \n", address);
    }
  }

  if (is_equal(command, "cat")) {
    if (!is_equal(argv[2], "--file")) {
      printf("invalid format");
      return 0;
    }
    char* address = argv[3];
    char* contents = cat(address);
    printf("%s", contents);
  }

  if (is_equal(command, "insert")) {
    char* address = argv[3];
    char* str = argv[5];
    int* pos = parse_pos(argv[7]);
    insert(address, pos[0], pos[1], str);
    printf("Insertion done \n");
  }

  if (is_equal(command, "paste")) {
    char* address = argv[3];
    int* pos = parse_pos(argv[5]);
    paste(address, pos[0], pos[1]);
    printf("Pasted \n");
  }

  if (is_equal(command, "remove")) {
    char* address = argv[3];
    int* pos = parse_pos(argv[5]);
    int size = atoi(argv[7]);
    bool backward = is_equal(argv[8], "-b");
    removestr(address, pos[0], pos[1], size, backward);
    printf("Removed \n");
  }

  if (is_equal(command, "copy") || is_equal(command, "cut")) {
    char* address = argv[3];
    int* pos = parse_pos(argv[5]);
    int size = atoi(argv[7]);
    bool backward = is_equal(argv[8], "-b");
    if (is_equal(command, "copy")) {
      copy(address, pos[0], pos[1], size, backward);
    }
    else {
      cut(address, pos[0], pos[1], size, backward);
    }
  }

  if (is_equal(command, "undo")) {
    undo(argv[3]);
    printf("(Un)done \n");
  }
}
