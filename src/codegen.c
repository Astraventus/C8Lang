#include "../include/codegen.h"
#include "../include/symtable.h"
#include "../include/error.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
    === Helpers ===
*/

int  node_size(const ASTNode *node) {
    switch (node->type) {
        case NODE_LABEL: 
            return 0;
        case NODE_SPRITE_DEF: 
            return node->sprite.count;
        case NODE_IF_EQ_IMM:
        case NODE_IF_EQ_REG:
        case NODE_IF_NEQ_IMM:
        case NODE_IF_NEQ_REG:
        case NODE_IFKEY:
        case NODE_IFNKEY:
            return 4;
        default: 
            return 2;
    }
}

void rom_emit(ROM *rom, uint16_t instr) {
    if (rom->size + 2 > ROM_MAX_SIZE) {
        error_fatal(0, "program exceeds maximum ROM size (0xFFF)");
    }
    rom->buf[rom->size++] = (instr >> 8) & 0xFF;
    rom->buf[rom->size++] = instr & 0xFF;
}


void rom_emit_byte(ROM *rom, uint8_t byte) {
    if (rom->size + 1 > ROM_MAX_SIZE) {
        error_fatal(0, "program exceeds maximum ROM size (0xFFF)");
    }
    rom->buf[rom->size++] = byte;
}

/*
    === PASS 1 ===
*/

static void pass1(const Program* prog, SymTable* st) {
    uint16_t offset = 0;

    for (int i = 0; i < prog->count; i++) {
        const ASTNode *n = &prog->nodes[i];
        if (n->type == NODE_LABEL) {
            symtable_define(st, n->label.name, ROM_BASE + offset, n->line);
        }
        else if (n->type == NODE_SPRITE_DEF) {
            symtable_define(st, n->sprite.name, ROM_BASE + offset, n->line);
            offset += node_size(n);
        }
        else {
            offset += node_size(n);
        }
    }
}

/*
    === PASS 2 ===
*/
static void pass2(const Program *prog, SymTable *st, ROM *rom) {
    for (int i = 0; i < prog->count; i++) {
        const ASTNode *n = &prog->nodes[i];

        switch (n->type) {
            case NODE_LABEL:
                /* Labels emit no code — they're just markers for pass1 */
                break;

            /* === Data movement === */
            case NODE_ASSIGN_IMM:    /* 6XNN: vX = NN */
                rom_emit(rom, 0x6000 | (n->assign_imm.reg << 8) | n->assign_imm.imm);
                break;

            case NODE_ASSIGN_REG:    /* 8XY0: vX = vY */
                rom_emit(rom, 0x8000 | (n->assign_reg.dst << 8) | (n->assign_reg.src << 4));
                break;

            /* === Arithmetic === */
            case NODE_ADD_IMM:       /* 7XNN: vX += NN */
                rom_emit(rom, 0x7000 | (n->add_imm.reg << 8) | n->add_imm.imm);
                break;

            case NODE_ADD_REG:       /* 8XY4: vX += vY */
                rom_emit(rom, 0x8000 | (n->add_reg.dst << 8) | (n->add_reg.src << 4) | 0x4);
                break;

            case NODE_SUB_REG:       /* 8XY5: vX -= vY */
                rom_emit(rom, 0x8000 | (n->sub_reg.dst << 8) | (n->sub_reg.src << 4) | 0x5);
                break;

            /* === Bitwise operations === */
            case NODE_OR_REG:        /* 8XY1: vX |= vY */
                rom_emit(rom, 0x8001 | (n->bitwise.dst << 8) | (n->bitwise.src << 4));
                break;

            case NODE_AND_REG:       /* 8XY2: vX &= vY */
                rom_emit(rom, 0x8002 | (n->bitwise.dst << 8) | (n->bitwise.src << 4));
                break;

            case NODE_XOR_REG:       /* 8XY3: vX ^= vY */
                rom_emit(rom, 0x8003 | (n->bitwise.dst << 8) | (n->bitwise.src << 4));
                break;

            case NODE_SHR:           /* 8XY6: vX >>= 1 (Y = X) */
                rom_emit(rom, 0x8006 | (n->shift.reg << 8) | (n->shift.reg << 4));
                break;

            case NODE_SHL:           /* 8XYE: vX <<= 1 (Y = X) */
                rom_emit(rom, 0x800E | (n->shift.reg << 8) | (n->shift.reg << 4));
                break;

            /* === Control flow === */
            case NODE_GOTO: {        /* 1NNN: unconditional jump */
                uint16_t addr = symtable_lookup(st, n->jump.target, n->line);
                rom_emit(rom, 0x1000 | (addr & 0xFFF));
                break;
            }

            /* === Memory (I register) === */
            case NODE_SET_I: {       /* ANNN: I = NNN (literal or label) */
                uint16_t addr = n->set_i.addr;
                if (n->set_i.label[0])
                    addr = symtable_lookup(st, n->set_i.label, n->line);
                rom_emit(rom, 0xA000 | (addr & 0xFFF));
                break;
            }

            case NODE_STORE:         /* FX55: store V0..VX to [I] */
                rom_emit(rom, 0xF055 | (n->mem.reg << 8));
                break;

            case NODE_LOAD:          /* FX65: load V0..VX from [I] */
                rom_emit(rom, 0xF065 | (n->mem.reg << 8));
                break;

            /* === Graphics === */
            case NODE_DRAW:          /* DXYN: draw sprite at (vX, vY), N rows */
                rom_emit(rom, 0xD000 | (n->draw.rx << 8) | (n->draw.ry << 4) | n->draw.n);
                break;

            case NODE_SPRITE_DEF:    /* Raw pixel data — not an instruction */
                for (int j = 0; j < n->sprite.count; j++)
                    rom_emit_byte(rom, n->sprite.bytes[j]);
                break;

            /* === Conditional jumps (4 bytes each: skip + goto) === */
            /* Pattern: emit "skip if condition is FALSE", then "goto target"
             * This works because CHIP-8 has conditional skip, not conditional branch. */

            case NODE_IF_EQ_IMM: {   /* goto target if vX == NN */
                uint16_t addr = symtable_lookup(st, n->if_eq_imm.target, n->line);
                rom_emit(rom, 0x4000 | (n->if_eq_imm.reg << 8) | n->if_eq_imm.imm);  /* skip if vX != NN */
                rom_emit(rom, 0x1000 | (addr & 0xFFF));                         /* goto target */
                break;
            }

            case NODE_IF_EQ_REG: {   /* goto target if vX == vY */
                uint16_t addr = symtable_lookup(st, n->if_eq_reg.target, n->line);
                rom_emit(rom, 0x9000 | (n->if_eq_reg.rx << 8) | (n->if_eq_reg.ry << 4)); /* skip if vX != vY */
                rom_emit(rom, 0x1000 | (addr & 0xFFF));
                break;
            }

            case NODE_IF_NEQ_IMM: {  /* goto target if vX != NN */
                uint16_t addr = symtable_lookup(st, n->if_neq_imm.target, n->line);
                rom_emit(rom, 0x3000 | (n->if_neq_imm.reg << 8) | n->if_neq_imm.imm); /* skip if vX == NN */
                rom_emit(rom, 0x1000 | (addr & 0xFFF));
                break;
            }

            case NODE_IF_NEQ_REG: {  /* goto target if vX != vY */
                uint16_t addr = symtable_lookup(st, n->if_neq_reg.target, n->line);
                rom_emit(rom, 0x5000 | (n->if_neq_reg.rx << 8) | (n->if_neq_reg.ry << 4)); /* skip if vX == vY */
                rom_emit(rom, 0x1000 | (addr & 0xFFF));
                break;
            }

            /* === Keyboard input === */
            case NODE_WAITKEY:       /* FX0A: block until key, store in vX */
                rom_emit(rom, 0xF00A | (n->waitkey.reg << 8));
                break;

            case NODE_IFKEY: {       /* goto target if key[vX] is pressed */
                uint16_t addr = symtable_lookup(st, n->key.target, n->line);
                rom_emit(rom, 0xE0A1 | (n->key.reg << 8));  /* skip if NOT pressed */
                rom_emit(rom, 0x1000 | (addr & 0xFFF));
                break;
            }

            case NODE_IFNKEY: {      /* goto target if key[vX] is NOT pressed */
                uint16_t addr = symtable_lookup(st, n->key.target, n->line);
                rom_emit(rom, 0xE09E | (n->key.reg << 8));  /* skip if pressed */
                rom_emit(rom, 0x1000 | (addr & 0xFFF));
                break;
            }

            /* === Timers === */
            case NODE_DELAYSET:      /* FX15: delay timer = vX */
                rom_emit(rom, 0xF015 | (n->timer_set.reg << 8));
                break;

            case NODE_DELAYGET:      /* FX07: vX = delay timer */
                rom_emit(rom, 0xF007 | (n->timer_get.reg << 8));
                break;

            case NODE_SOUNDSET:      /* FX18: sound timer = vX */
                rom_emit(rom, 0xF018 | (n->timer_set.reg << 8));
                break;
        }
    }
}

/*
    Public entry point
*/

void codegen_run(const Program *prog, SymTable *st, ROM *rom) {
    memset(rom, 0, sizeof(*rom));
    pass1(prog, st);
    pass2(prog, st, rom);
}

/*
    ROM output
*/

int rom_write(const ROM *rom, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) { perror(filename); return -1; }
    fwrite(rom->buf, 1, rom->size, f);
    fclose(f);
    return 0;
}

void rom_dump(const ROM *rom) {
    printf("=== ROM (%zu bytes) ===\n", rom->size);
    for (size_t i = 0; i < rom->size; i++) {
        if (i % 8 == 0)
            printf("  0x%03zX  ", ROM_BASE + i);
        printf("%02X ", rom->buf[i]);
        if (i % 8 == 7 || i == rom->size - 1)
            printf("\n");
    }
}