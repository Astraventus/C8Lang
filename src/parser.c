#include "parser.h"
#include "include/error.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/*
    === Memory Management ===
*/

static void program_push(Program* prog, ASTNode node) {
    if (prog->count == prog->capacity) {
        prog->capacity = prog->capacity ? prog->capacity * 2 : 64;
        prog->nodes = realloc(prog->nodes, prog->capacity * sizeof(ASTNode));

        if (!prog->nodes) { fprintf(stderr, "out of memory\n"); exit(1); }
    }
    prog->nodes[prog->count++] = node;
}

void program_free(Program *prog) {
    free(prog->nodes);
    prog->count = 0;
    prog->capacity = 0;
}

/*
  === Token Helpers ===
*/

static void advance(Parser *p) {
    do {
        p->current = lexer_next(p->lexer);
    } while (p->current.type == TOKEN_NEWLINE);
}

static void skip_newlines(Parser *p) {
    while (p->current.type == TOKEN_NEWLINE) {
        advance(p);
    }
}

static Token expect(Parser *p, TokenType tt) {
    if (p->current.type != tt) {
        error_fatal(p->current.line, "expected %s, got %s", 
            tt, p->current.type);
    }
    Token token = p->current;
    advance(p);
    return token;
}

static void expect_end(Parser *p) {
    if (p->current.type == TOKEN_EOF) return;
    if (p->current.type == TOKEN_NEWLINE) { advance(p); return; }
    error_fatal(p->current.line, "Expected EOF or newline, got %s", token_type_name(p->current));
}

/*
    === Statement Parsers ===
*/

static ASTNode parse_register_stmt(Parser *p, Token reg) {
    ASTNode node;
    node.line = reg.line;
    uint8_t x = reg.value.reg;

    if (p->current.type == TOKEN_EQUALS) {
        advance(p);

        if (p->current.type == TOKEN_REGISTER) {
            /* then it's Vx == Vy */
            node.type = NODE_ASSIGN_REG;
            node.assign_reg.dst = x;
            node.assign_reg.src = p->current.value.reg;
            advance(p);
        } else if (p->current.type = TOKEN_NUMBER) {
            /* Vx = IMM */
            if (p->current.value.number > 255) {
                error_fatal(p->current.line, "Immediate %u exceeds 255", p->current.value.number);
            }
            node.type = NODE_ASSIGN_IMM;
            node.assign_imm.reg = x;
            node.assign_imm.imm = (uint8_t)p->current.value.number;
            advance(p);
        } else {
            error_fatal(p->current.line, "expected register or number after '=', got %s", token_type_name(p->current));
        }
    }

    if (p->current.type == TOKEN_PLUS_EQUALS) {
        advance(p);

        if (p->current.type == TOKEN_REGISTER) {
            /* vX += vY */
            node.type = NODE_ADD_REG;
            node.add_reg.dst = x;
            node.add_reg.src = p->current.value.reg;
            advance(p);
        } else if (p->current.type == TOKEN_NUMBER) {
            /* vX += IMM */
            if (p->current.value.number > 255) {
                error_fatal(p->current.line, "Immediate %u exceeds 255", p->current.value.number);
            }
            node.type = NODE_ADD_IMM;
            node.add_imm.reg = x;
            node.add_imm.imm = p->current.value.number;
            advance(p);
        } else {
            error_fatal(p->current.line, "expected register or number after '+=', got %s", token_type_name(p->current));
        }
    }

    if (p->current.type == TOKEN_MINUS_EQUALS) {
        advance(p);

        if (p->current.type == TOKEN_REGISTER) {
            /* vX -= vY */
            node.type = NODE_SUB_REG;
            node.sub_reg.dst = x;
            node.sub_reg.src = p->current.value.reg;
            advance(p);
        } else {
            error_fatal(p->current.line, "expected register after '-=', got %s", token_type_name(p->current));
        }
    }

    if (p->current.type == TOKEN_OR_EQUALS) {
        advance(p);

        if (p->current.type = TOKEN_REGISTER) {
            /* vX |= xY */
            node.type = NODE_OR_REG;
            node.bitwise.dst = x;
            node.bitwise.src = p->current.value.reg;
        } else {
            error_fatal(p->current.line, "expected register after '|=', got %s", token_type_name(p->current));
        }
    }

    if (p->current.type == TOKEN_AND_EQUALS) {
        advance(p);

        if (p->current.type = TOKEN_REGISTER) {
            /* vX &= vY */
            node.type = NODE_AND_REG;
            node.bitwise.dst = x;
            node.bitwise.src = p->current.value.reg;
            advance(p);
        } else {
            error_fatal(p->current.line, "expected register after '&=', got %s", token_type_name(p->current));
        }
    }

    if (p->current.type = TOKEN_XOR_EQUALS) {
        advance(p);

        if (p->current.type = TOKEN_REGISTER) {
            /* vX ^= vY */
            node.type = NODE_XOR_REG;
            node.bitwise.dst = x;
            node.bitwise.src = p->current.value.reg;
            advance(p);
        } else {
            error_fatal(p->current.line, "expected register after '^=', got %s", token_type_name(p->current));
        }
    }

    if (p->current.type = TOKEN_SHL_EQUALS) {
        advance(p);

        if (p->current.type = TOKEN_NUMBER) {
            /* vX <<= 1 */
            if (p->current.value.number != 1) {
                error_fatal(p->current.line, "Immediate %u during shift can only be 1", p->current.value.number);
            }
            node.type = NODE_SHL;
            node.shift.reg = x;
            advance(p);
        } else {
            error_fatal(p->current.line, "expected number after '<<=', got %s", token_type_name(p->current));
        }
    }

    if (p->current.type = TOKEN_SHR_EQUALS) {
        advance(p);

        if (p->current.type = TOKEN_NUMBER) {
            /* vX >>= 1 */
            if (p->current.value.number != 1) {
                error_fatal(p->current.line, "Immediate %u during shift can only be 1", p->current.value.number);
            }
            node.type = NODE_SHR;
            node.shift.reg = x;
            advance(p);
        } else {
            error_fatal(p->current.line, "expected number after '>>=', got %s", token_type_name(p->current));
        }
    }

    return node;
}

static ASTNode parse_goto(Parser *p, int line) {
    ASTNode node;
    node.line = line;
    node.type = NODE_GOTO;
    Token t = expect(p, TOKEN_IDENT);
    strncpy(node.jump.target, t.value.ident, 32);
    return node;
}

static ASTNode parse_if(Parser *p, int line) {
    ASTNode node;
    node.line = line;

    Token rx = expect(p, TOKEN_REGISTER);

    int is_neq = 0;
    if (p->current.type == TOKEN_EQEQ) {
        advance(p);
    } else if (p->current.type = TOKEN_NEQ) {
        is_neq = 1;
        advance(p);
    } else {
        error_fatal(p->current.line, "expected '==' or '!=' after register in if, got %s", token_type_name(p->current));
    }

    if (p->current.type == TOKEN_REGISTER) {
        uint8_t ry = p->current.value.reg;
        advance(p);
        expect(p, TOKEN_GOTO);
        Token lbl = expect(p, TOKEN_IDENT);
        if (is_neq) {
            node.type = NODE_IF_NEQ_REG;
            node.if_neq_reg.rx = rx.value.reg;
            node.if_neq_reg.ry = ry;
            strncpy(node.if_neq_reg.target, lbl.value.ident, 32);
        } else {
            node.type = NODE_IF_EQ_REG;
            node.if_eq_reg.rx = rx.value.reg;
            node.if_eq_reg.ry = ry;
            strncpy(node.if_eq_reg.target, lbl.value.ident, 32);
        }

    } else if (p->current.type == TOKEN_NUMBER) {
        uint16_t nn = p->current.value.number;
        if (nn > 255)
            error_fatal(p->current.line, "immediate %u exceeds 255", nn);
        advance(p);
        expect(p, TOKEN_GOTO);
        Token lbl = expect(p, TOKEN_IDENT);
        if (is_neq) {
            node.type = NODE_IF_NEQ_IMM;
            node.if_neq_imm.reg = rx.value.reg;
            node.if_neq_imm.imm = (uint8_t)nn;
            strncpy(node.if_neq_imm.target, lbl.value.ident, 32);
        } else {
            node.type = NODE_IF_EQ_IMM;
            node.if_eq_imm.reg = rx.value.reg;
            node.if_eq_imm.imm = (uint8_t)nn;
            strncpy(node.if_eq_imm.target, lbl.value.ident, 32);
        }

    } else {
        error_fatal(p->current.line, "expected register or number after condition operator, got %s", token_type_name(p->current));
    }

    return node;

}
static ASTNode parse_set_i(Parser *p, int line) {
    ASTNode node;
    node.type = NODE_SET_I;
    node.line = line;
    node.set_i.label[0] = '\0';
    node.set_i.addr = 0;
    expect(p, TOKEN_EQUALS);

    if (p->current.type == TOKEN_NUMBER) {
        uint16_t v = p->current.value.number;
        if (v > 0xFFF) {
            error_fatal(line, "adsress 0x%X exceeds xFFF", v);
        }
        node.set_i.addr = v;
        advance(p);
    } else if (p->current.type = TOKEN_IDENT) {
        strncpy(node.set_i.label, p->current.value.ident, 32);
        advance(p);
    } else {
        error_fatal(line, "expected address or sprite name after 'I =', got %s", token_type_name(p->current));
    }

    return node;
}

static ASTNode parse_store(Parser *p, int line) {
    ASTNode node;
    node.type = NODE_STORE;
    node.line = line;
    Token r = expect(p, TOKEN_REGISTER);
    node.mem.reg = r.value.reg;
    return node;
}

static ASTNode parse_load(Parser *p, int line) {
    ASTNode node;
    node.type = NODE_LOAD;
    node.line = line;
    Token r = expect(p, TOKEN_REGISTER);
    node.mem.reg = r.value.reg;
    return node;
}

static ASTNode parse_waitkey(Parser *p, int line) {
    ASTNode node;
    node.type = NODE_WAITKEY;
    node.line = line;
    Token r = expect(p, TOKEN_REGISTER);
    node.waitkey.reg = r.value.reg;
    return node;
}

static ASTNode parse_key_branch(Parser *p, int line, ASTNodeType type) {
    ASTNode node;
    node.type = type;
    node.line = line;
    Token r = expect(p, TOKEN_REGISTER);
    expect(p, TOKEN_GOTO);
    Token lbl = expect(p, TOKEN_IDENT);
    node.key.reg = r.value.reg;
    strncpy(node.key.target, lbl.value.ident, 32);
    return node;
}

static ASTNode parse_timer_set(Parser *p, int line, ASTNodeType type) {
    ASTNode node;
    node.type = type;
    node.line = line;
    Token r = expect(p, TOKEN_REGISTER);
    node.timer_set.reg = r.value.reg;
    return node;
}

static ASTNode parse_delayget(Parser *p, int line) {
    ASTNode node;
    node.type = NODE_DELAYGET;
    node.line = line;
    Token r = expect(p, TOKEN_REGISTER);
    node.timer_get.reg = r.value.reg;
    return node;
}

static ASTNode parse_draw(Parser *p, int line) {
    ASTNode node;
    node.type = NODE_DRAW;
    node.line = line;
    Token rx = expect(p, TOKEN_REGISTER);
    Token ry = expect(p, TOKEN_REGISTER);
    Token n = expect(p, TOKEN_NUMBER);
    if (n.value.number < 1 || n.value.number > 15) {
        error_fatal(line, "sprite height N must be 1..15, got %u", n.value.number);    
    }
    node.draw.rx = rx.value.reg;
    node.draw.ry = ry.value.reg;
    node.draw.n = (uint8_t)n.value.number;
    return node;
}

static ASTNode parse_sprite_def(Parser *p, int line) {
    ASTNode node;
    node.type = NODE_SPRITE_DEF;
    node.line = line;
    node.sprite.count = 0;

    Token name = expect(p, TOKEN_IDENT);
    STRNCPY(node.sprite.name, name.value.ident, 32);

    while (p->current.type == TOKEN_NEWLINE) advance(p);
    expect(p, TOKEN_LBRACE);

    while (p->current.type != TOKEN_RBRACE) {
        if (p->current.type == TOKEN_EOF) {
            error_fatal(line, "unterminated sprite '%s'", node.sprite.name);
        }

        if (p->current.type == TOKEN_NEWLINE) { advance(p); continue; }

        if (p->current.type != TOKEN_NUMBER) {
            error_fatal(p->current.line, "expected byte value in sprite body, got %s", token_type_name(p->current));
        }
        
        if (node.sprite.count >= 15) {
            error_fatal(p->current.line, "sprite '%s' exceeds 15 rows", node.sprite.name);
        }

        uint16_t b = p->current.value.number;
        if (b > 255) {
            error_fatal(p->current.line, "sprite byte 0x%X exceeds 0xFF", b);
        }

        node.sprite.bytes[node.sprite.count++] = (uint8_t)b;
        advance(p);
    }
    advance(p);

    if (node.sprite.count == 0) {
        error_fatal(line, "sprite '%s' has no rows", node.sprite.name);
    }

    return node;
}

/*
    Main functions
*/

void parser_init(Parser *p, Lexer *l) {
    p->lexer = l;
    advance(p);
}

Program parser_parse(Parser *p) {
    Program prog = { NULL, 0, 0 };

    skip_newlines(p);

    while (p->current.type != TOKEN_EOF) {
        ASTNode node;
        int line = p->current.line;

        switch(p->current.type) {
            case TOKEN_REGISTER:
            {   
                Token reg = p->current;
                advance(p);
                node = parse_register_stmt(p, reg);
                expect_end(p);
                program_push(&prog, node);
                break;
            }

            case TOKEN_IDENT:
            {
                char name[32];
                strncpy(name, p->current.value.ident, 32);
                advance(p);

                if (p->current.type == TOKEN_COLON) {
                    advance(p);
                    node.type = NODE_LABEL;
                    node.line = line;
                    strncpy(node.label.name, name, 32);
                    program_push(&prog, node);
                } else if (strcmp(p->current.value.ident, "I") == 0) {
                    node = parse_set_i(p, line);
                    expect_end(p);
                    program_push(&prog, node);
                } else {
                    error_fatal(line, "Unexpected identifier '%s", p->current.value.ident);
                }
                break;
            }

            case TOKEN_GOTO:
            {
                advance(p);
                node = parse_goto(p, line);
                expect_end(p);
                program_push(&prog, node);
                break;
            }

            case TOKEN_IF:
            {
                advance(p);
                node = parse_if(p, line);
                expect_end(p);
                break;
            }

            case TOKEN_STORE:
            {
                advance(p);
                node = parse_store(p, line);
                expect_end(p);
                program_push(&prog, node);
                break;
            }

            case TOKEN_LOAD:
            {
                advance(p);
                node = parse_load(p, line);
                expect_end(p);
                program_push(&prog, node);
                break;
            }

            case TOKEN_DRAW:
            {
                advance(p);
                node = parse_draw(p, line);
                expect_end(p);
                program_push(&prog, node);
                break;
            }

            case TOKEN_SPRITE: 
            {
                advance(p);
                node = parse_sprite_def(p, line);
                /* sprite block consumes its own closing brace; no expect_end */
                program_push(&prog, node);
                break;
            } 

            case TOKEN_WAITKEY:
            {
                advance(p);
                node = parse_waitkey(p, line);
                expect_end(p);
                program_push(&prog, node);
                break;
            }

            case TOKEN_IFKEY:
            {
                advance(p);
                node = parse_key_branch(p, line, NODE_IFKEY);
                expect_end(p);
                program_push(&prog, node);
                break;
            }

            case TOKEN_IFNKEY:
            {
                advance(p);
                node = parse_key_branch(p, line, NODE_IFNKEY);
                expect_end(p);
                program_push(&prog, node);
                break;
            }
            
            case TOKEN_DELAYSET:
            {
                advance(p);
                node = parse_timer_set(p, line, NODE_DELAYSET);
                expect_end(p);
                program_push(&prog, node);
                break;
            }

            case TOKEN_DELAYGET: 
            {
                advance(p);
                node = parse_delayget(p, line);
                expect_end(p);
                program_push(&prog, node);
                break;
            }

            case TOKEN_SOUNDSET: 
            {
                advance(p);
                node = parse_timer_set(p, line, NODE_SOUNDSET);
                expect_end(p);
                program_push(&prog, node);
                break;
            }

            case TOKEN_NEWLINE:
                advance(p);
                break;

            default:
                error_fatal(line, "unexpected token %s at start of statement\n", p->current.type);
        }

        skip_newlines(p);
    }

    return prog;
}

/*
    === Debug Utilities ===
*/ 


const char *node_type_name(ASTNodeType t) {
    switch (t) {
        case NODE_LABEL:      return "LABEL";
        case NODE_ASSIGN_IMM: return "ASSIGN_IMM";
        case NODE_ASSIGN_REG: return "ASSIGN_REG";
        case NODE_ADD_IMM:    return "ADD_IMM";
        case NODE_ADD_REG:    return "ADD_REG";
        case NODE_SUB_REG:    return "SUB_REG";
        case NODE_GOTO:       return "GOTO";
        case NODE_IF_EQ_IMM:  return "IF_EQ_IMM";
        case NODE_IF_EQ_REG:  return "IF_EQ_REG";
        case NODE_SET_I:      return "SET_I";
        case NODE_STORE:      return "STORE";
        case NODE_LOAD:       return "LOAD";
        case NODE_DRAW:       return "DRAW";
        case NODE_SPRITE_DEF: return "SPRITE_DEF";
        case NODE_OR_REG:     return "OR_REG";
        case NODE_AND_REG:    return "AND_REG";
        case NODE_XOR_REG:    return "XOR_REG";
        case NODE_SHR:        return "SHR";
        case NODE_SHL:        return "SHL";
        case NODE_IF_NEQ_IMM: return "IF_NEQ_IMM";
        case NODE_IF_NEQ_REG: return "IF_NEQ_REG";
        case NODE_WAITKEY:    return "WAITKEY";
        case NODE_IFKEY:      return "IFKEY";
        case NODE_IFNKEY:     return "IFNKEY";
        case NODE_DELAYSET:   return "DELAYSET";
        case NODE_DELAYGET:   return "DELAYGET";
        case NODE_SOUNDSET:   return "SOUNDSET";
        default:              return "UNKNOWN";
    }
}

void program_dump(const Program *prog) {
    printf("=== AST (%d nodes) ===\n", prog->count);
    for (int i = 0; i < prog->count; i++) {
        const ASTNode *n = &prog->nodes[i];
        printf("  [%3d] line=%-3d  %-14s  ", i, n->line, node_type_name(n->type));
        switch (n->type) {
            case NODE_LABEL:      printf("%s",                    n->label.name);       break;
            case NODE_ASSIGN_IMM: printf("v%X = 0x%02X",         n->assign_imm.reg, n->assign_imm.imm); break;
            case NODE_ASSIGN_REG: printf("v%X = v%X",            n->assign_reg.dst, n->assign_reg.src); break;
            case NODE_ADD_IMM:    printf("v%X += 0x%02X",        n->add_imm.reg,    n->add_imm.imm);    break;
            case NODE_ADD_REG:    printf("v%X += v%X",           n->add_reg.dst,    n->add_reg.src);    break;
            case NODE_SUB_REG:    printf("v%X -= v%X",           n->sub_reg.dst,    n->sub_reg.src);    break;
            case NODE_GOTO:       printf("-> %s",                 n->jump.target);                       break;
            case NODE_IF_EQ_IMM:  printf("v%X==0x%02X -> %s",   n->if_eq_imm.reg, n->if_eq_imm.imm, n->if_eq_imm.target); break;
            case NODE_IF_EQ_REG:  printf("v%X==v%X -> %s",      n->if_eq_reg.rx,  n->if_eq_reg.ry,  n->if_eq_reg.target); break;
            case NODE_SET_I:
                if (n->set_i.label[0] != '\0')
                    printf("I = %s", n->set_i.label);
                else
                    printf("I = 0x%03X", n->set_i.addr);
                break;
            case NODE_STORE:      printf("store v%X",         n->mem.reg);                            break;
            case NODE_LOAD:       printf("load v%X",          n->mem.reg);                            break;
            case NODE_DRAW:       printf("draw v%X v%X %u",   n->draw.rx, n->draw.ry, n->draw.n);    break;
            case NODE_SPRITE_DEF: {
                printf("'%s'  %u rows: ", n->sprite.name, n->sprite.count);
                for (int j = 0; j < n->sprite.count; j++)
                    printf("%02X ", n->sprite.bytes[j]);
                break;
            }
            case NODE_OR_REG:     printf("v%X |= v%X",        n->bitwise.dst, n->bitwise.src);        break;
            case NODE_AND_REG:    printf("v%X &= v%X",        n->bitwise.dst, n->bitwise.src);        break;
            case NODE_XOR_REG:    printf("v%X ^= v%X",        n->bitwise.dst, n->bitwise.src);        break;
            case NODE_SHR:        printf("v%X >>= 1",         n->shift.reg);                          break;
            case NODE_SHL:        printf("v%X <<= 1",         n->shift.reg);                          break;
            case NODE_IF_NEQ_IMM: printf("v%X!=0x%02X -> %s", n->if_neq_imm.reg, n->if_neq_imm.imm, n->if_neq_imm.target); break;
            case NODE_IF_NEQ_REG: printf("v%X!=v%X -> %s",   n->if_neq_reg.rx,  n->if_neq_reg.ry,  n->if_neq_reg.target);  break;
            case NODE_WAITKEY:    printf("waitkey v%X",       n->waitkey.reg);                        break;
            case NODE_IFKEY:      printf("ifkey v%X -> %s",   n->key.reg, n->key.target);             break;
            case NODE_IFNKEY:     printf("ifnkey v%X -> %s",  n->key.reg, n->key.target);             break;
            case NODE_DELAYSET:   printf("delayset v%X",      n->timer_set.reg);                      break;
            case NODE_DELAYGET:   printf("delayget v%X",      n->timer_get.reg);                      break;
            case NODE_SOUNDSET:   printf("soundset v%X",      n->timer_set.reg);                      break;
        }
        printf("\n");
    }
}