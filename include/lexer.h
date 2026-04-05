#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdint.h>

typedef enum {
    /*  Screen                  */
    TOKEN_CLS,              /*  Clear screen                    */

    /*  Literals                */
    TOKEN_NUMBER,           /*  DEC: 42, HEX: 0x12              */
    TOKEN_REGISTER,         /*  v0-vf (represented as 1..15)    */
    TOKEN_IDENT,            /*  goto, if, label [name]:, etc.   */

    /*  Keywords                */
    TOKEN_GOTO,             /*  goto                            */
    TOKEN_IF,               /*  if                              */
    TOKEN_STORE,            /*  store                           */
    TOKEN_LOAD,             /*  load                            */
    TOKEN_DRAW,             /*  draw                            */
    TOKEN_SPRITE,           /*  sprite                          */

    /*  Input                   */
    TOKEN_WAITKEY,          /*  waitkey - FX0A block until key  */
    TOKEN_IFKEY,            /*  ifkey   - EX9A skip if key down */
    TOKEN_IFNKEY,           /*  ifnkey  - EXA1 skip if key up   */

    /*  Timers                  */
    TOKEN_DELAYSET,         /*  delayset - FX15 set delay timer */
    TOKEN_DELAYGET,         /*  delayget - FX07 get delay timer */
    TOKEN_SOUNDSET,         /*  soundset - FX18 set sound timer */

    /*  Operators/Punctuation   */
    TOKEN_EQUALS,           /*  =                               */
    TOKEN_PLUS_EQUALS,      /*  +=                              */
    TOKEN_MINUS_EQUALS,     /*  -=                              */
    TOKEN_OR_EQUALS,        /*  |=                              */
    TOKEN_AND_EQUALS,       /*  &=                              */
    TOKEN_XOR_EQUALS,       /*  ^=                              */
    TOKEN_SHR_EQUALS,       /*  >>=                             */
    TOKEN_SHL_EQUALS,       /*  <<=                             */    
    TOKEN_EQEQ,             /*  ==                              */
    TOKEN_NEQ,              /*  !=                              */
    TOKEN_COLON,            /*  : (label statement terminator)  */
    TOKEN_LBRACE,           /*  { (sprite body open)            */
    TOKEN_RBRACE,           /*  } (sprite body close)           */

    /*  Structured              */
    TOKEN_NEWLINE,          /*  logical end of statement        */
    TOKEN_EOF,              /*  end of the input                */
    
    /*  Errors                  */
    TOKEN_UNKNOWN,          /*  unrecognised character          */
} TokenType;

typedef struct {
    TokenType type;
    int line;

    union {
        uint8_t reg;
        uint16_t number;
        char ident[32];
    } value;
} Token;

typedef struct {
    const char* source;
    int pos;            /*  current character in the source */
    int line;
} Lexer;

/*
    === PUBLIC API ===
*/

/*
    initialise lexer
*/
void lexer_init(Lexer* l, const char* source);

/*
    Main function.
    Return the next token from the stream.
    Skip whitespaces and comments.
    Newlines are significant.
*/
Token lexer_next(Lexer* l);

/*
    For debugging.
    Return the name of the token.
*/
const char* token_type_name(TokenType t);

#endif