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

int main() {
  WINDOW* window = initscr();
  refresh();
  start_color();
  noecho();
  clear();

  enum Mode mode = NORMAL;

  char command[100];
  strcpy(command, "");

  char address[100];
  strcpy(address, "");
  char* contents = malloc(sizeof(char) * 1000000);

  while (true) {
    int maxx = getmaxx(window);
    int maxy = getmaxy(window);

    if (strlen(address) > 0) {
      clear();
      strcpy(contents, cat(remove_leading_slash(address)));
      char* line = malloc(sizeof(char) * 10000);
      int line_index = 0, i = 0;
      while (true) {
        char c = contents[i];
        if (c == '\n' || c == EOF || !c) {
          move(line_index, 0);
          printw(" %4d  %s", line_index + 1, line);
          strcpy(line, "");
          if (c == EOF || !c) break;
          line_index++;
          if (line_index >= maxy - 2) break;
        }
        else {
          strncat(line, &c, 1);
        }
        i++;
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
    attroff(COLOR_PAIR(2));

    move(maxy - 1, 0);
    if (strlen(command) > 0) {
      printw("%s", command);
    }

    if (mode == NORMAL) {
      char ch = getch();
      if (ch == '\n') {
        if (strlen(address) > 0) {
          if ((is_equal(command, ":undo") || is_equal(command, "u"))) {
            undo(address);
          }
          else if (is_equal(command, ":format") || is_equal(command, "=")) {
            char* contents = cat(remove_leading_slash(address));
            char* formatted_code = format(contents, 0, 0, 0);
            write_with_history(address, formatted_code);
          }
        }
        char rest[100];
        sscanf(command, ":open %[^\n]s", rest);
        if (strlen(rest) > 0) {
          strcpy(address, rest);
          clear();
        }
        strcpy(command, "");
      }
      else if (ch == 127) {
        command[strlen(command) - 1] = '\0';
        clear(); // don't know why this works
      }
      else if (ch == 27 && strlen(address) > 0) {
        mode = INSERT;
      }
      else {
        strncat(command, &ch, 1);
      }
    }
    else if (mode == INSERT) {
      move(0, 0);
      char ch = getch();
      if (ch == 27) {
        mode = INSERT;
      }
    }

    refresh();
  }

  endwin();
}
