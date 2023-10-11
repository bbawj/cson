#include "cson.h"

// TODO: store position and line to display as errors
int main(int argc, char *argv[]) {
  Cson c = {0};
  if (argc != 2) {
    fprintf(stderr, "Usage: cson <PATH>\n");
    exit(1);
  }
  Token *t = read_json(&c, argv[1]);
  if (!t) {
    fprintf(stderr, "Failed to parse json file\n");
    exit(1);
  }
  pretty_print(t, 0);
}
