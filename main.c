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
  for (int i = 0; i < 10000; i++) {
    lines[i] = malloc(sizeof(char) * 10000);
  }
  int lines_count = 0;
  int starting_line = 0;

  while (true) {
    int maxx = getmaxx(window);
    int maxy = getmaxy(window);

    if (strlen(address) > 0) {
      clear();
      if (strlen(contents) == 0) {
        strcpy(contents, cat(remove_leading_slash(address)));
        string_to_lines(contents, &lines_count, lines);
      }
      for (int i = starting_line; i < min(lines_count, starting_line + maxy - 2); i++) {
        move(i - starting_line, 0);
        printw(" %4d  %s", i + 1, lines[i]);
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
    else if (mode == VISUAL) {
      move(pos.line - starting_line, pos.col + 7);
      char ch = getch();
      if (ch == 'w') {
        pos.line--;
        if (pos.line - starting_line < 4) {
          starting_line = max(starting_line - 1, 0);
        }
        if (pos.line < 0) pos.line = 0;
        pos.col = min(pos.col, strlen(lines[pos.line]));
      }
      else if (ch == 'd') {
        pos.col = min(pos.col + 1, strlen(lines[pos.line]));
      }
      else if (ch == 's') {
        if (pos.line < lines_count - 1) {
          pos.line++;
          if (pos.line - starting_line >= maxy - 6) {
            starting_line++;
          }
        }
        pos.col = min(pos.col, strlen(lines[pos.line]));
      }
      else if (ch == 'a') {
        pos.col = max(pos.col - 1, 0);
      }
      else if (ch == '\n') {
        mode = INSERT;
      }
      else if (ch == 27) {
        mode = NORMAL;
      }
    }
    else if (mode == INSERT) {
      move(pos.line - starting_line, pos.col + 7);
      char ch = getch();
      if (ch == 27) {
        mode = VISUAL;
      }
      else if (ch == 127) {
        changed = true;
        lines[pos.line] = delete_char(lines[pos.line], pos.col - 1);
        pos.col = max(pos.col - 1, 0);
      }
      else {
        changed = true;
        lines[pos.line] = insert_char(lines[pos.line], pos.col, ch);
        pos.col = min(pos.col + 1, strlen(lines[pos.line]));
      }
    }

    refresh();
  }

  endwin();
}
