#include <stdbool.h>
#ifndef CSON_H
#define CSON_H

typedef enum {
  NULL,
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

bool scan_token(Token *res);
bool scan_array(Token *res);
bool scan_object(Token *res);
bool read_json(char *path, Token *t);

#endif // ! CSON_H
