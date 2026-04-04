#ifndef CODEGEN_H
#define CODEGEN_H

#include "parser.h"
#include "symtable.h"
#include <stdint.h>
#include <stddef.h>

#define ROM_BASE 0x200
#define ROM_MAX_SIZE 0xE00

/*
    Codegen performs two passes over the Program AST to produce CHIP-8 ROM.

    First pass populates SymTable and resolves lables - assigns addresses to them.
    Second pass emits instructions.
*/

typedef struct {
    uint8_t buf[ROM_MAX_SIZE];
    size_t size; /* bytes written so far */
} ROM;

/*
    === Public API ===
*/

/* Run both passes and populate `rom`.
 * Calls error_fatal() on any semantic or overflow error. */
void codegen_run(const Program *prog, SymTable* st, ROM* rom);

/* Write the ROM buffer to a file.  Returns 0 on success, -1 on error. */
int rom_write(const ROM* rom, const char* filename);

/* Debug: hex-dump the ROM to stdout. */
void rom_dump(const ROM* rom);

/*
    Internal helpers - exposed for possible unit tests
*/

/* Emit one 16-bit big-endian instruction into the ROM. */
void rom_emit(ROM *rom, uint16_t instr);

/* Compute the address a node starts at, given a base offset.
 * Returns how many bytes (2 or 4) the node occupies. */
int  node_size(const ASTNode *node);

#endif