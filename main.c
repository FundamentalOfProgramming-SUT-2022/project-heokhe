#include <ncurses.h>
#include <string.h>
#include "lib.c"

enum Mode {
  NORMAL = 0,
  INSERT = 1,
  VISUAL = 2
};
const char* mode_names[] = { "NORMAL", "INSERT", "VISUAL" };

int main() {
  enum Mode mode = NORMAL;
  char filename[100];

  WINDOW* window = initscr();
  refresh();
  start_color();
  noecho();
  clear();

  char command[100];
  strcpy(command, "");
  char address[100];

  while (true) {
    int maxx = getmaxx(window);
    int maxy = getmaxy(window);

    if (strlen(address) > 0) {
      move(0, 0);
      char* contents = cat(remove_leading_slash(address));
      printw("%s", contents);
    }

    move(maxy - 2, 0);
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    attron(COLOR_PAIR(1));
    attron(A_BOLD);
    printw(" %s ", mode_names[mode]);
    attroff(A_BOLD);
    attroff(COLOR_PAIR(1));
    printw(" ");
    if (strlen(address) == 0) {
      printw("(no open file)");
    }
    else {
      printw("%s", address);
    }
    printw(" ");

    move(maxy - 1, 0);
    if (strlen(command) > 0) {
      printw("%s", command);
    }

    char ch = getch();
    if (ch == '\n') {
      if (strlen(address) > 0 && (is_equal(command, ":undo") || is_equal(command, "u"))) {
        undo(address);
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
    else {
      strncat(command, &ch, 1);
    }

    refresh();
  }

  endwin();
  printf("command was: %s\n", command);
}
