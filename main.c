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
}
