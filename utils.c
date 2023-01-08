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
