#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <stdint.h>

/*
    Each statement in C8Lang corresponds to exactly one node type.
    The tree is intentionally flat and has no nested expressions.
*/
typedef enum {
    NODE_LABEL,         /* label:                              */
    NODE_ASSIGN_IMM,    /* vX = number                         */
    NODE_ASSIGN_REG,    /* vX = vY                             */
    NODE_ADD_IMM,       /* vX += number                        */
    NODE_ADD_REG,       /* vX += vY                            */
    NODE_SUB_REG,       /* vX -= vY                            */
    /* bitwise */
    NODE_OR_REG,        /* vX |= vY    — 8XY1                  */
    NODE_AND_REG,       /* vX &= vY    — 8XY2                  */
    NODE_XOR_REG,       /* vX ^= vY    — 8XY3                  */
    NODE_SHR,           /* vX >>= 1    — 8XY6 (Y ignored)      */
    NODE_SHL,           /* vX <<= 1    — 8XYE (Y ignored)      */
    /* control flow */
    NODE_GOTO,          /* goto label                          */
    NODE_CALL,          /* call label                          */
    NODE_RET,           /* ret                                 */
    NODE_IF_EQ_IMM,     /* if vX == number goto label          */
    NODE_IF_EQ_REG,     /* if vX == vY goto label              */
    NODE_IF_NEQ_IMM,    /* if vX != number goto label  — 4XNN  */
    NODE_IF_NEQ_REG,    /* if vX != vY goto label      — 9XY0  */
    /* memory */
    NODE_SET_I,         /* I = address                         */
    NODE_STORE,         /* store vX                            */
    NODE_LOAD,          /* load vX                             */
    /* graphics */
    NODE_DRAW,          /* draw vX vY N                        */
    NODE_SPRITE_DEF,    /* sprite name { byte... }             */
    /* input */
    NODE_WAITKEY,       /* waitkey vX  — FX0A                  */
    NODE_IFKEY,         /* ifkey vX goto label   — EX9E+1NNN   */
    NODE_IFNKEY,        /* ifnkey vX goto label  — EXA1+1NNN   */
    /* Screen */
    NODE_CLS,           /* Clear screen                        */
    /* timers */
    NODE_DELAYSET,      /* delayset vX — FX15                  */
    NODE_DELAYGET,      /* delayget vX — FX07                  */
    NODE_SOUNDSET,      /* soundset vX — FX18                  */
    /* random */
    NODE_RND,           /* rnd Vx, byte                        */
} ASTNodeType;

typedef struct {
    ASTNodeType type;
    int line;

    union {
        /* NODE_LABEL */
        struct { char name[32]; } label;

        /* NODE_ASSIGN_IMM */
        struct { uint8_t reg; uint8_t imm; } assign_imm;

        /* NODE_ASSIGN_REG */
        struct { uint8_t dst; uint8_t src; } assign_reg;

        /* NODE_ADD_IMM */
        struct { uint8_t reg; uint8_t imm; } add_imm;

        /* NODE_ADD_REG */
        struct { uint8_t dst; uint8_t src; } add_reg;

        /* NODE_SUB_REG */
        struct { uint8_t dst; uint8_t src; } sub_reg;

        /* NODE_OR_REG, NODE_AND_REG, NODE_XOR_REG: Vx op= Vy */
        struct { uint8_t dst; uint8_t src; } bitwise;

        /* NODE_SHR, NODE_SHL: Vx >>/<<= 1 */
        struct { uint8_t reg; } shift;

        /* NODE_GOTO */
        struct { char target[32]; } jump;

        /* NODE_CALL */
        struct { char target[32]; } call; 

        /* NODE_IF_EQ_IMM */ /* 0 - call, goto - 1 */
        struct { uint8_t reg; uint8_t imm; char target[32]; int call_or_goto; } if_eq_imm;

        /* NODE_IF_EQ_REG */ /* 0 - call, goto - 1 */
        struct { uint8_t rx; uint8_t ry; char target[32]; int call_or_goto; } if_eq_reg;

        /* NODE_IF_NEQ_IMM */ /* 0 - call, goto - 1 */
        struct { uint8_t reg; uint8_t imm; char target[32]; int call_or_goto; } if_neq_imm;

        /* NODE_IF_NEQ_REG */ /* 0 - call, goto - 1 */
        struct { uint8_t rx; uint8_t ry; char target[32]; int call_or_goto; } if_neq_reg;
        
        /* NODE_SET_I */
        struct { uint16_t addr; char label[32]; } set_i;

        /* NODE_STORE, NODE_LOAD */
        struct { uint8_t reg; } mem;

        /* NODE_DRAW */
        struct { uint8_t rx; uint8_t ry; uint8_t n; } draw;

        /* NODE_SPRITE_DEF: 
        form - [sprite name] { 0b1 0b2... }
        up to 15 rows according to chip-8 max sprite height.
        bytes emitted as a raw data into ROM; the name is entered into symtable
        pointing at the first byte;
        */
        struct {
            char name[32];
            uint8_t bytes[15];
            uint8_t count; // number of rows 1...15
        } sprite;

        /* NODE_WAITKEY */
        struct { uint8_t reg; } waitkey;
        
        /* NODE_IFKEY, NODE_IFNKEY */ /* 0 - call, goto - 1 */
        struct { uint8_t reg; char target[32]; int call_or_goto; } key;

        /* NODE_DELAYSET, NODE_SOUNDSET */
        struct { uint8_t reg; } timer_set;

        /* NODE_DELAYGET */
        struct { uint8_t reg; } timer_get;

        /* NODE_RND */
        struct { uint8_t reg; uint8_t byte; } random;
    };
} ASTNode;

typedef struct {
    ASTNode* nodes;
    int count;      // how much nodes are there
    int capacity;   // the capacity for nodes
} Program;

typedef struct {
    Lexer* lexer;
    Token current;
} Parser;

/*
    === PUBLIC API ===
*/

// Initialises parser
void parser_init(Parser* p, Lexer* l);

/*  Shall parse entire source into a Program */
Program parser_parse(Parser* p);

/*  Shall free memory owned by a program    */
void program_free(Program* prog);

/*  Shall dump every node to stdout     */
void program_dump(const Program* prog);

/*  Shall dump human readable name for a node type  */
const char* node_type_name(ASTNodeType t);


#endif