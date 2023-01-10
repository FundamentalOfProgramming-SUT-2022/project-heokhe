#include <stdbool.h>

bool is_equal(char* a, char* b) {
  return !strcmp(a, b);
}

void _touch(char* address) {
  FILE* file_pointer = fopen(address, "w");
  fclose(file_pointer);
}

bool _exists(char* address) {
  struct stat st = { 0 };
  return stat(address, &st) != -1;
}

char* remove_leading_slash(char* address) {
  int len = strlen(address);
  char* output = malloc(sizeof(char) * len);
  for (int i = 1; i < len; i++) {
    char ch = address[i];
    output[i - 1] = address[i];
  }
  return output;
}

char* read_clipboard() {
  FILE* temp_file = popen("pbpaste", "r");
  char* thing = malloc(sizeof(char) * 1e4);

  char ch;
  while ((ch = fgetc(temp_file)) != EOF) {
    strncat(thing, &ch, 1);
  }
  pclose(temp_file);
  return thing;
}

char* convert_backspaces(char* str) {
  int len = strlen(str);
  char* new_str = malloc(sizeof(char) * len);
  bool prev_is_backslash = false;
  for (int i = 0; i < len; i++) {
    char current = str[i];
    if (current == '\\') {
      if (prev_is_backslash) {
        strncat(new_str, "\\", 1);
        prev_is_backslash = false;
      }
      else {
        prev_is_backslash = true;
      }
    }
    else if (current == 'n' && prev_is_backslash) {
      strncat(new_str, "\n", 1);
      prev_is_backslash = false;
    }
    else {
      if (prev_is_backslash) {
        strncat(new_str, "\\", 1);
        prev_is_backslash = false;
      }
      strncat(new_str, &current, 1);
    }
  }
  return new_str;
}

int* parse_pos(char* pos) {
  int line = atoi(strtok(pos, ":"));
  int col = atoi(strtok(NULL, ":"));
  int* ptr;
  *(ptr + 0) = line;
  *(ptr + 1) = col;
  return ptr;
}
