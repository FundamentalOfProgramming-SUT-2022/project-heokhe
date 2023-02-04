#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include "lib.c"

enum Mode {
  NORMAL = 0,
  INSERT = 1,
  VISUAL = 2
};
const char* mode_names[] = { "NORMAL", "INSERT", "VISUAL" };

void string_to_lines(char* string, int* line_count, char* array[]) {
  char* line = malloc(sizeof(char) * 10000);
  int line_index = 0, i = 0;
  while (true) {
    char c = string[i];
    if (c == '\n' || c == EOF || !c) {
      strcpy(array[line_index], line);
      strcpy(line, "");
      if (c == EOF || !c) break;
      line_index++;
    }
    else {
      strncat(line, &c, 1);
    }
    i++;
  }
  *line_count = line_index + 1;
}

char* lines_to_string(int lines_count, char* lines[]) {
  char* string = malloc(sizeof(char) * 1000000);
  for (int i = 0; i < lines_count; i++) {
    if (i > 0) {
      aprintf(string, "\n");
    }
    aprintf(string, "%s", lines[i]);
  }
  return string;
}

char* insert_char(char* string, int index, char ch) {
  char* new_string = malloc(sizeof(char) * 10000);
  for (int i = 0; i <= index - 1; i++) {
    strncat(new_string, string + i, 1);
  }
  strncat(new_string, &ch, 1);
  for (int i = index; i < strlen(string); i++) {
    strncat(new_string, string + i, 1);
  }
  return new_string;
}

char* delete_char(char* string, int index) {
  char* new_string = malloc(sizeof(char) * 10000);
  for (int i = 0; i <= index - 1; i++) {
    strncat(new_string, string + i, 1);
  }
  for (int i = index + 1; i < strlen(string); i++) {
    strncat(new_string, string + i, 1);
  }
  return new_string;
}

struct Position {
  int line;
  int col;
};

int main() {
  WINDOW* window = initscr();
  refresh();
  start_color();
  noecho();
  clear();

  enum Mode mode = NORMAL;
  bool changed = false;
  struct Position pos = { 0, 0 };

  char command[100];
  strcpy(command, "");

  char address[100];
  strcpy(address, "");
  char* contents = malloc(sizeof(char) * 1000000);
  char* lines[10000];
  int offsets[10000];
  for (int i = 0; i < 10000; i++) {
    lines[i] = malloc(sizeof(char) * 10000);
  }
  int lines_count = 0;
  int starting_line = 0;

  while (true) {
    int maxx = getmaxx(window);
    int maxy = getmaxy(window);
    int code_width = maxx - 7;

    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    if (strlen(address) > 0) {
      clear();
      if (strlen(contents) == 0) {
        strcpy(contents, cat(remove_leading_slash(address)));
        string_to_lines(contents, &lines_count, lines);
      }
      offsets[0] = 0;
      for (int i = starting_line; i < min(lines_count, starting_line + maxy - 2); i++) {
        int len = strlen(lines[i]);
        int j;
        for (j = 0; j <= len; j += code_width) {
          move(i - starting_line + offsets[i] + j / code_width, 0);
          if (i == pos.line) attron(COLOR_PAIR(3));
          if (j == 0) {
            printw(" %4d  ", i + 1);
          }
          else {
            printw("       ");
          }
          attroff(COLOR_PAIR(3));
          printw("%s", lines[i] + j);
        }
        offsets[i + 1] = offsets[i] + j / code_width - 1;
      }
    }

    move(maxy - 2, 0);
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    attron(COLOR_PAIR(1));
    attron(A_BOLD);
    printw(" %s ", mode_names[mode]);
    attroff(A_BOLD);
    attroff(COLOR_PAIR(1));
    init_pair(2, COLOR_BLUE, COLOR_WHITE);
    attron(COLOR_PAIR(2));
    printw(" ");
    if (strlen(address) == 0) {
      printw("(no open file)");
    }
    else {
      printw("%s", address);
    }
    printw(" ");
    if (changed) {
      printw("+ ");
    }
    attroff(COLOR_PAIR(2));

    move(maxy - 1, 0);
    if (strlen(command) > 0) {
      printw("%s", command);
    }

    if (mode == NORMAL) {
      char ch = getch();
      if (ch == '\n') {
        if (!strlen(command)) {
          mode = VISUAL;
        }
        else {
          if (strlen(address) > 0) {
            if ((is_equal(command, ":undo") || is_equal(command, "u"))) {
              undo(address);
            }
            else if (is_equal(command, ":format") || is_equal(command, "=")) {
              char* contents = cat(remove_leading_slash(address));
              char* formatted_code = format(contents, 0, 0, 0);
              write_with_history(address, formatted_code);
            }
            else if (is_equal(command, ":save")) {
              write_with_history(address, lines_to_string(lines_count, lines));
              changed = false;
            }
            else if (strlen(address) > 0) {
              char save_destination[100];
              sscanf(command, ":saveas %[^\n]s", save_destination);
              if (strlen(save_destination) > 0) {
                if (!_exists(save_destination)) {
                  create_file(save_destination);
                }
                write_with_history(save_destination, lines_to_string(lines_count, lines));
                changed = false;
              }
            }
          }
          char rest[100];
          sscanf(command, ":open %[^\n]s", rest);
          if (strlen(rest) > 0) {
            if (strlen(address) > 0) {
              write_with_history(address, lines_to_string(lines_count, lines));
              changed = false;
            }
            strcpy(address, rest);
            strcpy(contents, "");
            pos.line = 0;
            pos.col = 0;
            clear();
          }
          strcpy(command, "");
        }
      }
      else if (ch == 127) {
        command[strlen(command) - 1] = '\0';
        clear(); // don't know why this works
      }
      else {
        strncat(command, &ch, 1);
      }
    }
    else {
      int real_line = pos.line + offsets[pos.line] + pos.col / code_width;
      move(real_line - starting_line, (pos.col % code_width) + 7);

      char ch = getch();
      if (mode == VISUAL) {
        if (ch == 'w') {
          if (pos.col < code_width) {
            pos.line = max(pos.line - 1, 0);
            pos.col = min(pos.col, strlen(lines[pos.line]));
          }
          else {
            pos.col -= code_width;
          }
        }
        else if (ch == 'd') {
          pos.col = min(pos.col + 1, strlen(lines[pos.line]));
        }
        else if (ch == 's') {
          int total = strlen(lines[pos.line]) / code_width;
          int current = (pos.col + 1) / code_width;
          if (current < total) {
            pos.col = min(pos.col + code_width, strlen(lines[pos.line]));
          }
          else {
            pos.line = min(pos.line + 1, lines_count - 1);
            pos.col = min(pos.col % code_width, strlen(lines[pos.line]));
          }
        }
        else if (ch == 'a') {
          pos.col = max(pos.col - 1, 0);
        }
        else if (ch == 'D') {
          char* line = lines[pos.line];
          int offset;
          for (offset = 1;; offset++) {
            if (pos.col + offset >= strlen(line)) break;
            if (line[pos.col + offset != ' '] && line[pos.col + offset - 1] == ' ') {
              break;
            }
          }
          pos.col = min(pos.col + offset, strlen(line));
        }
        else if (ch == 'A') {
          char* line = lines[pos.line];
          int offset;
          for (offset = 1;; offset++) {
            if (pos.col - offset <= 0) break;
            if (line[pos.col - offset - 1 != ' '] && line[pos.col - offset] == ' ') {
              break;
            }
          }
          pos.col = max(pos.col - offset, 0);
        }
        else if (ch == '\n') {
          mode = INSERT;
        }
        else if (ch == 27) {
          mode = NORMAL;
        }
      }
      else if (mode == INSERT) {
        // char ch = getch();
        if (!changed && ch != 27) changed = true;
        if (ch == 27) {
          mode = VISUAL;
        }
        else if (ch == 127) {
          if (is_equal(lines[pos.line], "")) {
            if (pos.line > 0) {
              for (int i = pos.line; i < lines_count - 1; i++) {
                lines[i] = lines[i + 1];
              }
              lines_count--;
              pos.line--;
              pos.col = strlen(lines[pos.line]);
            }
          }
          else {
            lines[pos.line] = delete_char(lines[pos.line], pos.col - 1);
            pos.col = max(pos.col - 1, 0);
          }
        }
        else if (ch == '\n') {
          for (int i = lines_count - 1; i >= pos.line; i--) {
            lines[i + 1] = lines[i];
          }
          lines[pos.line + 1] = malloc(sizeof(char) * 10000);
          lines_count++;
          for (int i = pos.col;; i++) {
            char c = lines[pos.line][i];
            if (!c) break;
            strncat(lines[pos.line + 1], &c, 1);
          }
          char* new_line = malloc(sizeof(char) * 10000);
          for (int i = 0; i < pos.col; i++) {
            new_line[i] = lines[pos.line][i];
          }
          lines[pos.line] = new_line;
        }
        else {
          lines[pos.line] = insert_char(lines[pos.line], pos.col, ch);
          pos.col = min(pos.col + 1, strlen(lines[pos.line]));
        }
      }

      int next_real_line = pos.line + offsets[pos.line] + pos.col / code_width;
      if (next_real_line > real_line) {
        while (next_real_line - starting_line >= maxy - 6) {
          starting_line++;
        }
      }
      else if (next_real_line < real_line) {
        while (next_real_line - starting_line <= 4 && starting_line > 0) {
          starting_line--;
        }
      }
    }
    refresh();
  }
  endwin();
}
