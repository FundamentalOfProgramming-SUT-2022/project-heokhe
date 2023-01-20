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

bool is_directory(char* address) {
  struct stat st;
  stat(address, &st);
  return S_ISDIR(st.st_mode);
}

char* remove_leading_slash(char* address) {
  if (address[0] != '/') return address;
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

void write_to_clipboard(char* str) {
  char command[10000];
  sprintf(command, "printf \"%s\" | pbcopy", str);
  system(command);
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

int get_index_of_pos(char* str, int line, int col) {
  int i = 0, current_line = 0;
  while (true) {
    char ch = str[i];
    if (ch == '\n') {
      current_line++;
    }
    if (current_line == line) break;
    i++;
  }
  return i + col;
}

int* parse_pos(char* pos) {
  int line = atoi(strtok(pos, ":"));
  int col = atoi(strtok(NULL, ":"));
  int* ptr = malloc(2 * sizeof(int));
  *(ptr + 0) = line;
  *(ptr + 1) = col;
  return ptr;
}

void remove_index(char* str, int index) {
  memmove(&str[index], &str[index + 1], strlen(str) - index);
}

void write_to_file(char* address, char* content) {
  address = remove_leading_slash(address);
  FILE* file = fopen(address, "w");
  fprintf(file, "%s", content);
  fclose(file);
}

char* get_prev_version_name(char* address) {
  int len = strlen(address);
  int history_name_len = len + 100;
  char* history_file_address = malloc(history_name_len * sizeof(char));
  strcat(history_file_address, ".undo_history");
  strcat(history_file_address, address);
  for (int i = 0; i < 5; i++) {
    remove_index(history_file_address, 13);
  }
  return history_file_address;
}

bool get_flag(int argc, char* argv[], char* flag) {
  char with_dashes[100];
  sprintf(with_dashes, "--%s", flag);
  for (int i = 2; i < argc; i++) {
    if (is_equal(argv[i], with_dashes)) {
      return true;
    }
  }
  return false;
}

char* get_argument(int argc, char* argv[], char* argument_name) {
  char with_dashes[100];
  sprintf(with_dashes, "--%s", argument_name);
  for (int i = 2; i < argc - 1; i++) {
    if (is_equal(argv[i], with_dashes)) {
      return argv[i + 1];
    }
  }
  return "";
}
