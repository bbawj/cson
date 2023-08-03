#include "cson.h"
#include <ctype.h>
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
  return c.b[++c.cur];
}

char peek() {
  if (c.cur == c.size) {
    return '\0';
  }
  return c.b[c.cur];
}

char peek_next() {
  if (c.cur + 1 >= c.size) {
    return '\0';
  }
  return c.b[c.cur + 1];
}

const char *translate_tokentype(TOKEN_TYPE t) {
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

bool scan_number(Token *res) {
  bool neg = false;
  bool fraction = false;
  bool exp = false;
  int start = c.cur;
  size_t len = 1;

  while (true) {
    char cur = peek();
    if (cur == '-') {
      if (neg)
        return false;
      char next = peek_next();
      if (!isdigit(next))
        return false;
      neg = true;
      advance();
      len++;
    } else if (isdigit(cur)) {
      while (true) {
        char next = advance();

        if (isdigit(next)) {
          len++;
        } else if (next == '.') {
          if (fraction || !isdigit(peek_next())) {
            return false;
          }
          fraction = true;
          len++;
        } else if (next == 'e' || next == 'E') {
          if (exp)
            return false;
          char next_next = peek_next();
          if (next_next == '+' || next_next == '-') {
            advance();
            len++;
            char next_next_next = peek_next();
            if (!isdigit(next_next_next))
              return false;
          } else if (!isdigit(next_next))
            return false;

          exp = true;
          len++;
        } else {
          char *text = malloc(len + 1);
          memcpy(text, &c.b[start], len);
          text[len] = '\0';
          res->type = NUMBER;
          res->text = text;
          return true;
        }
      }
      return false;
    }
  }
}

bool scan_string(Token *res) {
  char cur = peek();
  if (cur != '"')
    return false;

  size_t len = 1;
  size_t start = c.cur;

  while (true) {
    char next = advance();
    len++;

    if (next == '"') {
      char *text = malloc(len + 1);
      memcpy(text, &c.b[start], len);
      text[len] = '\0';
      res->type = STRING;
      res->text = text;
      advance();
      return true;
    } else if (next == '\\') {
      char next_next = advance();
      len++;
      switch (next_next) {
      case '"':
      case '\\':
      case '/':
      case 'b':
      case 'f':
      case 'n':
      case 'r':
      case 't':
      case 'u':
        break;
      default:
        return false;
      }
    }
  }
}

bool scan_special(Token *res, TOKEN_TYPE special) {
  const char *s = translate_tokentype(special);
  size_t len = strlen(s);
  if (c.cur + len > c.size)
    return false;
  char *temp = malloc(len + 1);
  memcpy(temp, &c.b[c.cur], len);
  temp[len] = '\0';

  if (strcmp(temp, s) == 0) {
    res->type = special;
    res->text = temp;
    c.cur += len;
    return true;
  }
  return false;
}

bool scan_token(Token *res) {
  while (true) {
    char cur = peek();
    switch (cur) {
    case '\t':
    case '\n':
    case '\r':
    case ' ':
      advance();
      continue;
    case '[':
      advance();
      return scan_array(res);
    case '{':
      break;
    case '"':
      return scan_string(res);
    case 't':
      return scan_special(res, TRUE);
    case 'f':
      return scan_special(res, FALSE);
    case 'n':
      return scan_special(res, 0);
    default:
      return scan_number(res);
    }
  }
  return false;
}

void skip_whitespace() {
  while (true) {
    char cur = peek();
    switch (cur) {
    case '\t':
    case '\n':
    case '\r':
    case ' ':
      advance();
      continue;
    default:
      return;
    }
  }
}

bool scan_array(Token *res) {
  Token *root = res;
  char cur = peek();
  while (cur != ']') {
    Token *next = malloc(sizeof(Token));
    next->child = NULL;
    next->next = NULL;

    if (scan_token(next)) {
      if (root->child == NULL) {
        root->child = next;
      } else {
        res->next = next;
      }
      res = next;
    } else {
      free(next);
      return false;
    }
    skip_whitespace();
    cur = peek();
    if (cur == ',') {
      cur = advance();
    }
  }
  root->type = ARRAY;
  return true;
}

bool scan_object(Token *res) {}

void pretty_print(Token *root, int depth) {
  if (root == NULL)
    return;

  while (root != NULL) {
    for (int i = 0; i < depth; i++) {
      printf("\t");
    }
    printf("type: %s, text: %s\n", translate_tokentype(root->type), root->text);

    if (root->child != NULL)
      pretty_print(root->child, depth + 1);
    root = root->next;
  }
}

// TODO: store position and line to display as errors
int main(void) {
  init();
  open_file("json.json");
  Token t = {0};
  if (!scan_token(&t)) {
    if (c.cur == c.size) {
      printf("Reached EOF");
      exit(1);
    }
    printf("An error while parsing char '%c' at position %zu", c.b[c.cur],
           c.cur);
    exit(0);
  }
  pretty_print(&t, 0);
}
