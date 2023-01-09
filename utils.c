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

