#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "constants.c"
#include "utils.c"

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
  if (!_exists(UNDO_HISTORY)) {
    mkdir(UNDO_HISTORY, 0700);
  }
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
    remove_index(contents, backward ? (index - size + 1) : index);
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

char* result(uint8_t code, char* output) {
  char* r = malloc(sizeof(char) * (1e6 + 10));
  r[0] = code + 1; // since we can't store zero in a string
  strncat(r, output, strlen(output));
  return r;
}

#define ok(msg) result(0, msg)

char* handle(int argc, char* argv[]);

char* arman(int arman_index, int argc, char* argv[]) {
  int left_argc = arman_index;
  int right_argc = argc - arman_index;
  char* left_argv[left_argc];
  char* right_argv[right_argc];
  left_argv[0] = argv[0];
  right_argv[0] = argv[0];
  for (int i = 1; i < left_argc; i++) {
    left_argv[i] = argv[i];
  }
  for (int i = 1; i < right_argc; i++) {
    right_argv[i] = argv[i + arman_index];
  }

  char* left_output = handle(left_argc, left_argv);
  if ((int)left_output[0] != 1) {
    return left_output;
  }
  remove_index(left_output, 0);

  right_argc += 2;
  for (int i = right_argc; i > 5; i--) {
    right_argv[i] = right_argv[i - 2];
  }
  right_argv[4] = "--str";
  right_argv[5] = left_output;

  char* right_output = handle(right_argc, right_argv);
  return right_output;
}

char* tree(char* address, int depth, int max_depth) {
  address = remove_leading_slash(address);

  char* o = malloc(sizeof(char) * 1e5);

  DIR* directory = opendir(address);
  struct dirent* file;
  while ((file = readdir(directory)) != NULL) {
    char name[file->d_namlen];
    strcpy(name, file->d_name);
    if (is_equal(name, ".") || is_equal(name, "..")) continue;

    char* file_address = malloc(sizeof(char) * 200);
    strncat(file_address, address, strlen(address));
    strncat(file_address, "/", 1);
    strncat(file_address, name, strlen(name));

    for (int i = 0; i < depth - 1; i++) strncat(o, "  ", 2);
    if (depth > 0) strncat(o, "└─", 6);

    strcat(o, file_address);
    strncat(o, "\n", 1);

    bool next_depth_is_allowed = max_depth == -1 || depth < max_depth;
    if (next_depth_is_allowed && is_directory(file_address)) {
      strcat(o, tree(file_address, depth + 1, max_depth));
    }
  }
  closedir(directory);
  return o;
}

int find(char* address, char* expression, int at, bool byword, bool only_count) {
  char* contents = cat(address);
  int len = strlen(contents), ex_len = strlen(expression);
  int index = -1, word_index = -1;
  int count = 0, matched_count = 0;

  bool begins_with_wildcard = false, ends_with_wildcard = false;
  if (expression[0] == '*') {
    begins_with_wildcard = true;
    remove_index(expression, 0);
    ex_len--;
  }
  if (expression[ex_len - 1] == '*') {
    if (expression[ex_len - 2] == '\\') {
      remove_index(expression, ex_len - 2);
      ex_len--;
    }
    else {
      ends_with_wildcard = true;
      remove_index(expression, ex_len - 1);
      ex_len--;
    }
  }

  for (int i = 0, w = 0, ws = 0; i < len; i++) {
    if ((begins_with_wildcard || ends_with_wildcard) && matched_count == 0 && w == word_index && i > ws) continue;

    if (contents[i] == expression[matched_count]) {
      matched_count++;
      if (matched_count == ex_len) {
        if (ends_with_wildcard && (contents[i + 1] == ' ' || i == len - 1)) {
          matched_count = 0;
        }
        else {
          index = begins_with_wildcard ? ws : (i - ex_len + 1);
          word_index = w;
          count++;
          matched_count = 0;
          if (!only_count && at == count - 1) {
            break;
          }
        }
      }
    }
    // else if (matched_count > 0 && i && contents[i] == contents[i - 1]) {
    //   printf("%d?\n", matched_count);
    //   continue;
    // }
    else {
      matched_count = 0;
    }

    if (contents[i] == ' ') {
      w++;
      ws = i + 1;
    }
  }

  if (only_count) return count;
  if (at != count - 1) return -1;
  return byword ? word_index : index;
}

int replace(char* address, char* str1, char* str2, int at, bool all) {
  char* output = malloc(sizeof(char) * 1e6);
  int index = find(address, str1, at, false, false);
  if (index == -1) return 1;
  char* contents = cat(address);
  int len = strlen(str1);
  for (int i = 0; i < index; i++) {
    strncat(output, &contents[i], 1);
  }
  for (int i = 0; i < strlen(str2); i++) {
    strncat(output, &str2[i], 1);
  }
  for (int i = index + 1; i < strlen(contents); i++) {
    strncat(output, &contents[i], 1);
  }
  write_with_history(address, output);
  return 0;
}

char* grep(int address_count, char* addresses[], char* pattern, bool count_only, bool names_only) {
  char* output = malloc(sizeof(char) * 1e7);

  int matched_lines_count = 0;
  for (int address_index = 0; address_index < address_count; address_index++) {
    char* address = addresses[address_index];
    char* address_without_root = address + strlen(ROOT) + 1;
    if (address[0] == '/') address_without_root++;

    FILE* file = fopen(remove_leading_slash(address), "r");
    char* line = malloc(sizeof(char) * 1000);
    bool file_includes_pattern = false;

    char current;
    while (true) {
      current = fgetc(file);
      if (current == '\n' || current == EOF) {
        if (strincludes(line, pattern)) {
          matched_lines_count++;
          file_includes_pattern = true;
          if (names_only) {
            break;
          }
          if (!count_only && !names_only) {
            aprintf(output, "%s: %s\n", address_without_root, line);
          }
        }
        memset(line, 0, strlen(line));
        if (current == EOF) {
          break;
        }
      }
      else {
        strncat(line, &current, 1);
      }
    }

    if (names_only) {
      aprintf(output, "%s\n", address_without_root);
    }

    fclose(file);
  }

  if (count_only) {
    aprintf(output, "%d", matched_lines_count);
  }

  return output;
}

char* handle(int argc, char* argv[]) {
  int arman_index = -1;
  for (int i = 0; i < argc; i++) {
    if (is_equal(argv[i], "=D")) {
      arman_index = i;
      break;
    }
  }

  if (arman_index != -1) {
    return arman(arman_index, argc, argv);
  }

  char* command = argv[1];
  if (is_equal(command, "createfile")) {
    if (!is_equal(argv[2], "--file")) {
      return result(1, "invalid format");
    }
    char* address = argv[3];
    int status = create_file(address);
    if (!status) {
      return ok("");
    }
    else {
      // TODO: give an actual explanation
      return result(2, "");
    }
  }

  if (is_equal(command, "cat")) {
    if (!is_equal(argv[2], "--file")) {
      return result(1, "invalid format");
    }
    char* address = argv[3];
    char* contents = cat(address);
    return ok(contents);
  }

  if (is_equal(command, "insert")) {
    char* address = argv[3];
    char* str = argv[5];
    int* pos = parse_pos(argv[7]);
    insert(address, pos[0], pos[1], str);
    return ok("");
  }

  if (is_equal(command, "paste")) {
    char* address = argv[3];
    int* pos = parse_pos(argv[5]);
    paste(address, pos[0], pos[1]);
    return ok("");
  }

  if (is_equal(command, "remove")) {
    char* address = argv[3];
    int* pos = parse_pos(argv[5]);
    int size = atoi(argv[7]);
    bool backward = is_equal(argv[8], "-b");
    removestr(address, pos[0], pos[1], size, backward);
    return ok("");
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
    return ok("");
  }

  if (is_equal(command, "undo")) {
    undo(argv[3]);
    return ok("");
  }

  if (is_equal(command, "tree")) {
    int max_depth = atoi(argv[2]);
    if (max_depth < -1) return result(1, "Depth should be at least -1");
    return ok(tree(ROOT, 0, max_depth));
  }

  if (is_equal(command, "find")) {
    char* address = get_argument(argc, argv, "file");
    char* expression = get_argument(argc, argv, "str");
    char* at = get_argument(argc, argv, "at");
    if (is_equal(at, "")) at = "0";
    bool byword = get_flag(argc, argv, "byword");
    bool count = get_flag(argc, argv, "count");
    int output = find(address, expression, atoi(at), byword, count);
    if (output == -1) {
      return result(1, "not found");
    }
    else {
      char s[1000];
      sprintf(s, "%d", output);
      return ok(s);
    }
  }

  if (is_equal(command, "replace")) {
    char* address = get_argument(argc, argv, "file");
    char* str1 = get_argument(argc, argv, "str1");
    char* str2 = get_argument(argc, argv, "str2");
    char* at = get_argument(argc, argv, "at");
    bool all = get_flag(argc, argv, "all");
    if (is_equal(at, "")) at = "0";
    return result(replace(address, str1, str2, atoi(at), all), "");
  }

  if (is_equal(command, "grep")) {
    char* addresses[argc];
    int addresses_count = 0;
    int flag_index;
    for (int i = 2; i < argc; i++) {
      if (is_equal(argv[i], "--files")) {
        flag_index = i;
        break;
      }
    }
    for (int i = flag_index + 1; i < argc; i++) {
      if (argv[i][0] == '-') break;
      addresses[i - flag_index - 1] = argv[i];
      addresses_count++;
    }

    return ok(grep(addresses_count, addresses, get_argument(argc, argv, "str"), get_flag(argc, argv, "c"), get_flag(argc, argv, "l")));
  }

  return result(3, "");
}

int main(int argc, char* argv[]) {
  char* res = handle(argc, argv);
  int status = res[0] - 1; // status code starts from zero
  remove_index(res, 0);
  if (strlen(res)) {
    printf("%s\n", res);
  }
  return status;
}
