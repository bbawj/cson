#include <stdbool.h>
#ifndef CSON_H
#define CSON_H

typedef enum {
  NULL,
  ELEMENT,
  OBJECT,
  ARRAY,
  STRING,
  CHARACTER,
  ESCAPE,
  NUMBER,
  TRUE,
  FALSE,
  MEMBER,
  VALUE,
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

#endif // ! CSON_H
