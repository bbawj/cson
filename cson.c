#include "cson.h"
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

typedef struct {
  char *line;
  size_t len;
} Buffer;

struct CSON {
  char *b;
  size_t cap;
  size_t size;
  size_t cur;
};

struct CSON c;

void init() {
  c.cap = 1024 * 1024;
  c.size = 0;
  c.cur = 0;
  c.b = malloc(c.cap);
}

void append_line(char *line, size_t len) {
  if (c.size + len > c.cap) {
    c.cap *= 2;
    c.b = realloc(c.b, sizeof(char) * c.cap);
  }

  memcpy(&c.b[c.size], line, len);
  c.size += len;
}

void open_file(char *filename) {
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    exit(0);
  }
  char *line = NULL;
  size_t len = 0;
  ssize_t nread;
  while ((nread = getline(&line, &len, fp)) != -1) {
    append_line(line, len);
  }
  free(line);
  fclose(fp);
}

char advance() {
  if (c.cur == c.size) {
    return '\0';
  }
  return c.b[c.cur++];
}

char peek() {
  if (c.cur == c.size) {
    return '\0';
  }
  return c.b[c.cur];
}

bool scan_number(Token *res) {
  char cur = peek();
  if (isdigit(cur)) {
    int start = c.cur;
    size_t len = 1;
    while (true) {
      char next = advance();
      if (!isdigit(next)) {
        char *text = malloc(len + 1);
        memcpy(text, &c.b[start], len);
        text[len] = '\0';
        res->type = NUMBER;
        res->text = text;
      }
      len++;
    }
  }
  return false;
}

bool scan_token(Token *res) {
  while (true) {
    char cur = peek();
    switch (cur) {
    case '\0':
    case '\n':
    case '\r':
    case ' ':
      advance();
      continue;
    default:
      if (cur == '-') {
        return scan_number(res);
      }
      return scan_number(res);
    }
  }
  return false;
}
char *translate_tokentype(TOKEN_TYPE t) {
  switch (t) {
  case 0:
    return "null";
  case ELEMENT:
    return "element";
  case OBJECT:
    return "object";
  case ARRAY:
    return "array";
  case STRING:
    return "string";
  case CHARACTER:
    return "char";
  case ESCAPE:
    return "escape";
  case NUMBER:
    return "number";
  case TRUE:
    return "true";
  case FALSE:
    return "false";
  case MEMBER:
    return "member";
  case VALUE:
    return "value";
  default:
    return NULL;
  }
}

int main(void) {
  init();
  open_file("json.json");
  Token t;
  if (!scan_token(&t)) {
    printf("An error occurred at col %zu", c.cur);
    exit(0);
  }
  printf("type: %s, text: %s", translate_tokentype(t.type), t.text);
}
