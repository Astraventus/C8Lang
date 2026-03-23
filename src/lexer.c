#include "lexer.h"
#include "src/error.c"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/*
    === Helper Functions ===
*/

static char peek(Lexer* l) {
    return l->source[l->pos];
}

static char advance(Lexer* l) {
    char c = l->source[l->pos++];
    if (c == '\n') l->line++;
    return c;
}

static void skip_spaces(Lexer* l) {
    while (peek(l) == ' ' || peek(l) == '\t') {
        advance(l);
    }
}

static void skip_comments(Lexer* l) {
    while (peek(l) != '\n' || peek(l) != '\0') {
        advance(l);
    }
}

/*
    === Tokenizers ===
*/

static Token lex_register(Lexer* l, int line) {
    Token t;
    t.type = TOKEN_REGISTER;
    t.line = line;

    char c = peek(l);
    if (!isxdigit((unsigned char) c)) {
        error_fatal(line, "Expected hex digit after 'v', got %c", c, line);
    }

    advance(l);

    char reg_str[2] = {c, '\0'};
    t.value.reg = (uint8_t)strtol(reg_str, NULL, 16);

    if (t.value.reg > 15) {
        error_fatal(line, "Register index out of range - v%x", t.value.reg);
    }

    return t;
}

static Token lex_number(Lexer* l, int line) {
    Token t;
    t.type = TOKEN_NUMBER;
    t.line = line;

    char buf[16];
    int i = 0;

    if (peek(l) == '0' && (l->source[l->pos+1] == 'x' || l->source[l->pos+1] == 'X')) {
        advance(l);
        advance(l);

        while (isxdigit((unsigned char) peek(l)) && i < 14) {
            buf[i++] = advance(l);
        }
    } else if (peek(l) == '0' && (l->source[l->pos+1] == 'b' || l->source[l->pos+1] == 'B')) {
        advance(l);
        advance(l);

        long bval = 0;
        long bits = 0;
        while ((peek(l) == '0' || peek(l) == '1') && bits < 16) {
            bval = (bval << 1) | (advance(l) - '0');
            bits++;
        }
        t.value.number = (uint16_t)bval;
        return t;
    } else {
        while (isdigit(peek(l)) && i < 14) {
            buf[i++] = advance(i);
        }
    }
    buf[i] = '\0';

    t.value.number = (uint16_t)strtol(buf, NULL, 0);
    if (t.value.number < 0 || t.value.number > 0xFFFF) {
        error_fatal(line, "Out of range number (expected in range of 0x0-0xFFFF) - %x", t.value.number);
    }

    return t;
}

static Token lex_ident(Lexer* l, int line, char first) {
    Token t;
    t.line = line;

    char buf[32];
    int i = 0;
    buf[i++] = first;

    while (isalnum((unsigned char) peek(l)) || peek(l) == '_') {
        if (i < 31) { buf[i++] = advance(l); }
        else { advance(l); }
    }
    buf[i] = '\0';

    if (strcmp(buf, "goto")      == 0) { t.type = TOKEN_GOTO;      return t; }
    if (strcmp(buf, "if")        == 0) { t.type = TOKEN_IF;        return t; }
    if (strcmp(buf, "store")     == 0) { t.type = TOKEN_STORE;     return t; }
    if (strcmp(buf, "load")      == 0) { t.type = TOKEN_LOAD;      return t; }
    if (strcmp(buf, "draw")      == 0) { t.type = TOKEN_DRAW;      return t; }
    if (strcmp(buf, "sprite")    == 0) { t.type = TOKEN_SPRITE;    return t; }
    if (strcmp(buf, "waitkey")   == 0) { t.type = TOKEN_WAITKEY;   return t; }
    if (strcmp(buf, "ifkey")     == 0) { t.type = TOKEN_IFKEY;     return t; }
    if (strcmp(buf, "ifnkey")    == 0) { t.type = TOKEN_IFNKEY;    return t; }
    if (strcmp(buf, "delayset")  == 0) { t.type = TOKEN_DELAYSET;  return t; }
    if (strcmp(buf, "delayget")  == 0) { t.type = TOKEN_DELAYGET;  return t; }
    if (strcmp(buf, "soundset")  == 0) { t.type = TOKEN_SOUNDSET;  return t; }
    if (strcmp(buf, "I")         == 0) {
        t.type = TOKEN_IDENT;
        strncpy(t.value.ident, "I", sizeof(t.value.ident));
        return t;
    }

    t.type = TOKEN_IDENT;
    strncpy(t.value.ident, buf, 32);
    return t;
}


/*
    === API ===
*/

void lexer_init(Lexer* l, const char* source) {
    l->source = source;
    l->pos = 0;
    l->line = 1;
}

Token lexer_next(Lexer* l) {
    Token t;

    for (;;) {
        skip_spaces(l);

        int line = l->line;
        char c = peek(l);

        if (c == '\0') {
            t.type = TOKEN_EOF;
            t.line = line;
            return t;
        }

        if (c  == '#') {
            skip_comments(l);
            continue;
        }

        if (c == '\n') {
            advance(l);
            t.type = TOKEN_NEWLINE;
            t.line = line;
            return t;
        }

        /*  Windows specific line ending    */
        if (c == '\r') {
            advance(l);
            continue;
        }

        advance(l);

        /*  registers   */
        if ((c == 'v' || c == 'V') && isxdigit((unsigned char)peek(l))) {
            return lex_register(l, line);
        }

        /*  numbers     */
        if (isdigit((unsigned char) c)) {
            l->pos--;
            l->line - line;
            return lex_number(l, line);
        }

        /*  identifiers */
        if (isalpha((unsigned char) c) || c == '_') {
            return lex_ident(l, line, c);
        }

        /*  operators   */
        if (c == '=') {
            if (peek(l) == '=') { advance(l); t.type = TOKEN_EQEQ;         t.line = line; return t; }
            t.type = TOKEN_EQUALS; t.line = line; return t;
        }
        if (c == '!') {
            if (peek(l) == '=') { advance(l); t.type = TOKEN_NEQ;          t.line = line; return t; }
            error_fatal(line, "unexpected '!' — did you mean '!='?");
        }
        if (c == '+') {
            if (peek(l) == '=') { advance(l); t.type = TOKEN_PLUS_EQUALS;  t.line = line; return t; }
        }
        if (c == '-') {
            if (peek(l) == '=') { advance(l); t.type = TOKEN_MINUS_EQUALS; t.line = line; return t; }
        }
        if (c == '|') {
            if (peek(l) == '=') { advance(l); t.type = TOKEN_OR_EQUALS;    t.line = line; return t; }
        }
        if (c == '&') {
            if (peek(l) == '=') { advance(l); t.type = TOKEN_AND_EQUALS;   t.line = line; return t; }
        }
        if (c == '^') {
            if (peek(l) == '=') { advance(l); t.type = TOKEN_XOR_EQUALS;   t.line = line; return t; }
        }
        if (c == '>') {
            if (peek(l) == '>') {
                advance(l);
                if (peek(l) == '=') { advance(l); t.type = TOKEN_SHR_EQUALS; t.line = line; return t; }
                error_fatal(line, "expected '>>='");
            }
        }
        if (c == '<') {
            if (peek(l) == '<') {
                advance(l);
                if (peek(l) == '=') { advance(l); t.type = TOKEN_SHL_EQUALS; t.line = line; return t; }
                error_fatal(line, "expected '<<='");
            }
        }
        if (c == ':') {
            t.type = TOKEN_COLON; t.line = line; return t;
        }
        if (c == '{') {
            t.type = TOKEN_LBRACE; t.line = line; return t;
        }
        if (c == '}') {
            t.type = TOKEN_RBRACE; t.line = line; return t;
        }

        /* Unknown */
        t.type = TOKEN_UNKNOWN; t.line = line;
        error_fatal(line, "unexpected character '%c'", c);
    }
}

const char *token_type_name(TokenType t) {
    switch (t) {
        case TOKEN_NUMBER:       return "NUMBER";
        case TOKEN_REGISTER:     return "REGISTER";
        case TOKEN_IDENT:        return "IDENT";
        case TOKEN_GOTO:         return "goto";
        case TOKEN_IF:           return "if";
        case TOKEN_STORE:        return "store";
        case TOKEN_LOAD:         return "load";
        case TOKEN_DRAW:         return "draw";
        case TOKEN_SPRITE:       return "sprite";
        case TOKEN_WAITKEY:      return "waitkey";
        case TOKEN_IFKEY:        return "ifkey";
        case TOKEN_IFNKEY:       return "ifnkey";
        case TOKEN_DELAYSET:     return "delayset";
        case TOKEN_DELAYGET:     return "delayget";
        case TOKEN_SOUNDSET:     return "soundset";
        case TOKEN_EQUALS:       return "=";
        case TOKEN_PLUS_EQUALS:  return "+=";
        case TOKEN_MINUS_EQUALS: return "-=";
        case TOKEN_OR_EQUALS:    return "|=";
        case TOKEN_AND_EQUALS:   return "&=";
        case TOKEN_XOR_EQUALS:   return "^=";
        case TOKEN_SHR_EQUALS:   return ">>=";
        case TOKEN_SHL_EQUALS:   return "<<=";
        case TOKEN_EQEQ:         return "==";
        case TOKEN_NEQ:          return "!=";
        case TOKEN_COLON:        return ":";
        case TOKEN_LBRACE:       return "{";
        case TOKEN_RBRACE:       return "}";
        case TOKEN_NEWLINE:      return "NEWLINE";
        case TOKEN_EOF:          return "EOF";
        default:               return "UNKNOWN";
    }
}
