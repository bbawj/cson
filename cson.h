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
  struct Token *child;
  struct Token *next;
} Token;

bool scan_token(Token *res);
bool scan_array(Token *res);

#endif // ! CSON_H
