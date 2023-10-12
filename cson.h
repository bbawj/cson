#ifndef CSON_H
#define CSON_H

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CSON_ALLOC
#define CSON_ALLOC malloc
#endif

#ifndef CSON_PRINT
#define CSON_PRINT printf
#endif

typedef struct {
  char *line;
  size_t len;
} Buffer;

typedef struct {
  char *b;
  size_t cap;
  size_t size;
  size_t cur;
} Cson;

typedef enum {
  NULL_VALUE,
  OBJECT,
  ARRAY,
  STRING,
  NUMBER,
  TRUE,
  FALSE,
} TOKEN_TYPE;

typedef struct Token {
  TOKEN_TYPE type;
  char *text;
  // If exists, the child is the 'value' of the current 'key' token
  struct Token *child;
  // If exists, holds the pointer to the next token at the same level
  struct Token *next;
} Token;

void log_error(Cson *c, char *message) {
  char buf[256];
  snprintf(buf, sizeof(buf), "ERROR: invalid token '%c'. %s", c->b[c->cur],
           message);
  fprintf(stderr, "%s", buf);
}

void init(Cson *c) {
  c->cap = 1024 * 1024;
  c->size = 0;
  c->cur = 0;
  c->b = CSON_ALLOC(c->cap);
  assert(c->b);
}

void append_line(Cson *c, char *line, size_t len) {
  if (c->size + len > c->cap) {
    c->cap *= 2;
    c->b = realloc(c->b, sizeof(char) * c->cap);
    assert(c->b);
  }

  memcpy(&c->b[c->size], line, len);
  c->size += len;
}

void open_file(Cson *c, const char *filename) {
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    exit(0);
  }
  char *line = NULL;
  size_t len = 0;
  size_t nread;
  while ((nread = getline(&line, &len, fp)) != -1) {
    append_line(c, line, nread);
  }
  free(line);
  fclose(fp);
}

char advance(Cson *c) {
  if (c->cur == c->size) {
    return '\0';
  }
  return c->b[++c->cur];
}

char peek(Cson *c) {
  if (c->cur == c->size) {
    return '\0';
  }
  return c->b[c->cur];
}

char peek_next(Cson *c) {
  if (c->cur + 1 >= c->size) {
    return '\0';
  }
  return c->b[c->cur + 1];
}

const char *translate_tokentype(TOKEN_TYPE t) {
  switch (t) {
  case 0:
    return "null";
  case OBJECT:
    return "object";
  case ARRAY:
    return "array";
  case STRING:
    return "string";
  case NUMBER:
    return "number";
  case TRUE:
    return "true";
  case FALSE:
    return "false";
  default:
    return NULL;
  }
}

bool scan_number(Cson *c, Token *res) {
  bool neg = false;
  bool fraction = false;
  bool exp = false;
  int start = c->cur;
  size_t len = 1;

  while (true) {
    char cur = peek(c);
    if (cur == '-') {
      if (neg)
        return false;
      char next = peek_next(c);
      if (!isdigit(next))
        return false;
      neg = true;
      advance(c);
      len++;
    } else if (isdigit(cur)) {
      while (true) {
        char next = advance(c);

        if (isdigit(next)) {
          len++;
        } else if (next == '.') {
          if (fraction || !isdigit(peek_next(c))) {
            return false;
          }
          fraction = true;
          len++;
        } else if (next == 'e' || next == 'E') {
          if (exp)
            return false;
          char next_next = peek_next(c);
          if (next_next == '+' || next_next == '-') {
            advance(c);
            len++;
            char next_next_next = peek_next(c);
            if (!isdigit(next_next_next))
              return false;
          } else if (!isdigit(next_next))
            return false;

          exp = true;
          len++;
        } else {
          char *text = CSON_ALLOC(len + 1);
          memcpy(text, &c->b[start], len);
          text[len] = '\0';
          res->type = NUMBER;
          res->text = text;
          return true;
        }
      }
      return false;
    } else {
      log_error(c, "Expected '-' or digit\n");
      return false;
    }
  }
}

void skip_whitespace(Cson *c) {
  while (true) {
    char cur = peek(c);
    switch (cur) {
    case '\t':
    case '\n':
    case '\r':
    case ' ':
    case '\0':
      advance(c);
      continue;
    default:
      return;
    }
  }
}

char scan_next(Cson *c) {
  advance(c);
  skip_whitespace(c);
  return peek(c);
}

bool scan_string(Cson *c, Token *res) {
  char cur = peek(c);
  if (cur != '"')
    return false;

  size_t len = 0;
  size_t start = c->cur + 1;

  while (true) {
    char next = advance(c);
    len++;

    if (next == '"') {
      if (len == 0) {
        res->text = NULL;
      } else {
        char *text = CSON_ALLOC(len);
        assert(text);
        memcpy(text, &c->b[start], len - 1);
        text[len - 1] = '\0';
        res->text = text;
      }
      res->type = STRING;
      return true;
    } else if (next == '\\') {
      char next_next = advance(c);
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
        fprintf(stderr, "Invalid escape sequence '%c'\n", next_next);
        return false;
      }
    }
  }
}

bool scan_special(Cson *c, Token *res, TOKEN_TYPE special) {
  const char *s = translate_tokentype(special);
  size_t len = strlen(s);
  if (c->cur + len > c->size)
    return false;
  char *temp = CSON_ALLOC(len + 1);
  assert(temp);
  memcpy(temp, &c->b[c->cur], len);
  temp[len] = '\0';

  if (strcmp(temp, s) == 0) {
    res->type = special;
    res->text = temp;
    c->cur += len;
    return true;
  }
  return false;
}

bool scan_token(Cson *c, Token *res);

bool scan_array(Cson *c, Token *res) {
  assert(peek(c) == '[');
  Token *root = res;
  char cur = scan_next(c);
  while (cur != ']') {
    Token *next = CSON_ALLOC(sizeof(Token));
    next->child = NULL;
    next->next = NULL;
    next->text = NULL;
    next->type = 0;

    if (scan_token(c, next)) {
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
    skip_whitespace(c);
    cur = peek(c);
    if (cur == ',') {
      cur = scan_next(c);
    }
  }
  root->type = ARRAY;
  advance(c);
  return true;
}

bool scan_object(Cson *c, Token *res) {
  assert(peek(c) == '{');
  Token *prev = res;
  char cur = scan_next(c);
  while (cur != '}') {
    Token *next = CSON_ALLOC(sizeof(Token));
    assert(next);
    next->child = NULL;
    next->next = NULL;
    next->text = NULL;
    next->type = 0;

    if (!scan_string(c, next)) {
      free(next);
      return false;
    }
    cur = scan_next(c);
    if (cur != ':') {
      free(next);
      fprintf(stderr, "Expected ':' after key\n");
      return false;
    }
    advance(c);
    Token *child = CSON_ALLOC(sizeof(Token));
    assert(child);
    child->child = NULL;
    child->next = NULL;
    child->text = NULL;
    child->type = 0;
    if (!scan_token(c, child)) {
      free(child);
      return false;
    }
    prev->next = next;
    next->child = child;
    prev = next;

    cur = scan_next(c);
    if (cur == ',') {
      advance(c);
    }
  }
  advance(c);
  res->type = OBJECT;
  return true;
}

bool scan_token(Cson *c, Token *res) {
  while (true) {
    skip_whitespace(c);
    char cur = peek(c);
    switch (cur) {
    case '\0':
      return false;
    case '[':
      return scan_array(c, res);
    case '{':
      return scan_object(c, res);
    case '"':
      return scan_string(c, res);
    case 't':
      return scan_special(c, res, TRUE);
    case 'f':
      return scan_special(c, res, FALSE);
    case 'n':
      return scan_special(c, res, 0);
    default:
      return scan_number(c, res);
    }
  }
  return false;
}

void pretty_print(Token *root, int depth) {
  if (root == NULL)
    return;

  while (root != NULL) {
    for (int i = 0; i < depth; i++) {
      CSON_PRINT("\t");
    }
    CSON_PRINT("type: %s, text: %s\n", translate_tokentype(root->type),
               root->text);

    if (root->child != NULL)
      pretty_print(root->child, depth + 1);
    root = root->next;
  }
}

Token *parse_json(Cson *c) {
  Token *t = CSON_ALLOC(sizeof(Token));
  t->next = NULL;
  t->child = NULL;
  t->text = NULL;
  t->type = 0;
  if (!scan_token(c, t)) {
    if (c->cur == c->size) {
      CSON_PRINT("Reached EOF");
      return NULL;
    }
    CSON_PRINT("An error while parsing char '%c' at position %zu", c->b[c->cur],
               c->cur);
    return NULL;
  }
  return t;
}

Token *parse_json_file(Cson *c, const char *path) {
  init(c);
  open_file(c, path);
  return parse_json(c);
}

void free_tokens(Token *t) {
  if (t == NULL)
    return;
  while (t != NULL) {
    free_tokens(t->child);
    Token *temp = t->next;
    free(t);
    t = temp;
  }
}

#endif // ! CSON_H
