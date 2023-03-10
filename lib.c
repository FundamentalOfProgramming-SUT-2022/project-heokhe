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
  address = remove_leading_slash(address);

  if (_exists(address)) return 1;

  int len = strlen(address);

  char* current_address = malloc(sizeof(char) * len);
  char* segment = malloc(sizeof(char) * 100);
  int segment_index = 0;

  for (int i = 0; i < len; i++) {
    char current = address[i];

    if (current == '/') {
      if (segment_index > 0) {
        strncat(current_address, "/", 1);
      }

      strncat(current_address, segment, strlen(segment));
      if (!_exists(current_address)) {
        mkdir(current_address, 0700);
      }

      strcpy(segment, "");
      segment_index++;
    }
    else {
      strncat(segment, &current, 1);
    }
  }

  aprintf(current_address, "/%s", segment);

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
  char* prev_name = get_prev_version_name(address);
  if (!_exists(prev_name)) {
    create_file(prev_name);
  }
  write_to_file(prev_name, cat(address));
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

  if (
    !is_equal(right_argv[0], "find") &&
    !is_equal(right_argv[0], "insert") &&
    !is_equal(right_argv[0], "grep")
    ) {
    return result(10, "right-hand side command doesn't accept --str");
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

#define END_OF_FIND -2
int* find(char* address, char* expression, int at, bool byword, bool only_count, bool all) {
  char* contents = cat(address);
  int len = strlen(contents), ex_len = strlen(expression);
  int index = -1, word_index = -1;
  int count = 0, matched_count = 0;

  bool should_not_stop = only_count || all;
  int* output = malloc(sizeof(int) * 1000);
  int output_size = 0;

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
          if (all) {
            int to_be_added = byword ? word_index : index;
            if (output_size == 0 || output[output_size - 1] != to_be_added) {
              output[output_size] = to_be_added;
              output_size++;
            }
          }
          if (!should_not_stop && at == count - 1) {
            break;
          }
        }
      }
    }
    // this makes the code be able to find "ab" in "aab"
    else if (matched_count > 0 && contents[i] == contents[i - matched_count]) {
      continue;
    }
    else {
      matched_count = 0;
    }

    if (contents[i] == ' ') {
      w++;
      ws = i + 1;
    }
  }

  if (only_count) {
    output[0] = count;
    output[1] = END_OF_FIND;
  }
  else if (!should_not_stop) {
    if (at != count - 1) output[0] = -1;
    else output[0] = byword ? word_index : index;
    output[1] = END_OF_FIND;
  }
  else {
    output[output_size] = END_OF_FIND;
  }
  return output;
}

int replace(char* address, char* str1, char* str2, int at, bool all) {
  if (all) {
    while (true) {
      if (replace(address, str1, str2, 0, false)) {
        break;
      }
    }
    return 0;
  }

  char* output = malloc(sizeof(char) * 1e6);
  int index = find(address, str1, at, false, false, false)[0];
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

char* compare(char* a, char* b) {
  char* output = malloc(sizeof(char) * 10000000);

  FILE* file_a = fopen(remove_leading_slash(a), "r");
  FILE* file_b = fopen(remove_leading_slash(b), "r");
  int i = 0;
  int len_a, len_b;
  char* line_a, * line_b;

  while (true) {
    line_a = fgetl(file_a);
    line_b = fgetl(file_b);
    len_a = strlen(line_a);
    len_b = strlen(line_b);

    if (len_a == 0 || len_b == 0) break;

    if (!is_equal(line_a, line_b)) {
      aprintf(output, "========= #%d =========\n%s\n%s\n", i + 1, line_a, line_b);
    }

    i++;
  }


  if (len_a != 0 || len_b != 0) {
    FILE* larger_file = len_b == 0 ? file_a : file_b;
    char* rest = malloc(sizeof(char) * 1000000);
    char* line = len_b == 0 ? line_a : line_b;
    int start = i, end = i;
    while (true) {
      if (strlen(line) == 0) break;
      aprintf(rest, "%s\n", line);
      line = fgetl(larger_file);
      end++;
    }
    aprintf(output, ">>>>>>>>> #%d - #%d >>>>>>>>>\n%s", start + 1, end, rest);
  }

  fclose(file_a);
  fclose(file_b);

  return output;
}

char* format(char* code, int from, int to, int depth) {
  char* indent = malloc(sizeof(char) * 1000);
  for (int i = 0; i < depth; i++)
    strncat(indent, "  ", 2);

  char* output = malloc(sizeof(char) * 100000);

  int balance = 0;
  int starting_index = -1, ending_index = -1;
  for (int i = from; !to || i <= to; i++) {
    char c = code[i];
    if (!c) break;

    if (c == '{') {
      balance++;
      if (balance == 1) {
        starting_index = i + 1;
      }
    }
    else if (c == '}') {
      balance--;
      if (balance == 0) {
        ending_index = i - 1;

        while (true) {
          int len2 = strlen(output);
          if (output[len2 - 1] == '\n' || output[len2 - 1] == ' ') {
            remove_index(output, len2 - 1);
          }
          else {
            break;
          }
        }

        int olen = strlen(output);
        if (olen > 0 && output[olen - 1] == '}') {
          aprintf(output, "\n%s", indent);
        }
        else if (olen > 0 && output[olen - 1] != ' ') {
          aprintf(output, " ");
        }
        else {
          aprintf(output, "%s", indent);
        }

        char* inner = format(code, starting_index, ending_index, depth + 1);
        if (is_equal(inner, "")) {
          aprintf(output, "{\n%s}", indent);
        }
        else {
          aprintf(output, "{\n%s\n%s}", inner, indent);
        }
      }
    }
    else if (balance == 0) {
      if (i == from) {
        aprintf(output, "%s", indent);
      }
      if (code[i - 1] == '}') {
        aprintf(output, "\n");
        aprintf(output, "%s", indent);
      }
      strncat(output, &c, 1);
    }
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
    char* address = get_argument(argc, argv, "file");
    int status = create_file(address);
    if (!status) {
      return ok("");
    }
    else if (status == 1) {
      return result(1, "file already exists");
    }
    else {
      // TODO: give an actual explanation
      return result(status, "");
    }
  }

  if (is_equal(command, "cat")) {
    char* address = get_argument(argc, argv, "file");
    char* contents = cat(address);
    return ok(contents);
  }

  if (is_equal(command, "insert")) {
    char* address = get_argument(argc, argv, "file");
    char* str = get_argument(argc, argv, "str");
    int* pos = parse_pos(get_argument(argc, argv, "pos"));
    insert(address, pos[0], pos[1], str);
    return ok("");
  }

  if (is_equal(command, "paste")) {
    char* address = get_argument(argc, argv, "file");
    int* pos = parse_pos(get_argument(argc, argv, "pos"));
    paste(address, pos[0], pos[1]);
    return ok("");
  }

  if (is_equal(command, "remove")) {
    char* address = get_argument(argc, argv, "file");
    int* pos = parse_pos(get_argument(argc, argv, "pos"));
    int size = atoi(get_argument(argc, argv, "size"));
    bool backward = get_flag(argc, argv, "b") || !get_flag(argc, argv, "f");
    removestr(address, pos[0], pos[1], size, backward);
    return ok("");
  }

  if (is_equal(command, "copy") || is_equal(command, "cut")) {
    char* address = get_argument(argc, argv, "file");
    int* pos = parse_pos(get_argument(argc, argv, "pos"));
    int size = atoi(get_argument(argc, argv, "size"));
    bool backward = get_flag(argc, argv, "b") || !get_flag(argc, argv, "f");
    if (is_equal(command, "copy")) {
      copy(address, pos[0], pos[1], size, backward);
    }
    else {
      cut(address, pos[0], pos[1], size, backward);
    }
    return ok("");
  }

  if (is_equal(command, "undo")) {
    undo(get_argument(argc, argv, "file"));
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
    bool all = get_flag(argc, argv, "all");
    int* output = find(address, expression, atoi(at), byword, count, all);
    if (output[0] == -1) {
      return result(1, "not found");
    }
    else {
      char s[1000000];
      for (int i = 0; output[i] != END_OF_FIND; i++) {
        aprintf(s, "%d ", output[i]);
      }
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

  if (is_equal(command, "compare")) {
    return ok(compare(argv[2], argv[3]));
  }

  if (is_equal(command, "format")) {
    char* address = argv[2];
    char* contents = cat(remove_leading_slash(address));
    char* formatted_code = format(contents, 0, 0, 0);
    write_with_history(address, formatted_code);
    return ok("");
  }

  return result(3, "");
}

// int main(int argc, char* argv[]) {
//   char* res = handle(argc, argv);
//   int status = res[0] - 1; // status code starts from zero
//   remove_index(res, 0);
//   if (strlen(res)) {
//     printf("%s\n", res);
//   }
//   return status;
// }
