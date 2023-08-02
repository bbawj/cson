#ifndef CSON_H
#define CSON_H

typedef struct {
  char *whitespace;
} Whitespace;

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

typedef struct {
  TOKEN_TYPE type;
  char *text;
} Token;

#endif // ! CSON_H
