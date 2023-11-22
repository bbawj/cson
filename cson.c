#define CSON_IMPLEMENTATION
#include "cson.h"

// TODO: store position and line to display as errors
int main(int argc, char *argv[]) {
  Cson c = {0};
  if (argc != 2) {
    fprintf(stderr, "Usage: cson <PATH>\n");
    exit(1);
  }
  Token *json = parse_json_file(&c, argv[1]);
  if (!json) {
    fprintf(stderr, "Failed to parse json file\n");
    exit(1);
  }
  pretty_print(json, 0);
  free_tokens(json);
}
