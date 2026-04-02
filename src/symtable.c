#include "include/symtable.h"
#include "error.h"
#include <string.h>
#include <stdio.h>

void symtable_init(SymTable* st) {
    memset(st, 0, sizeof(*st));
}

void symtable_define(SymTable* st, const char* name, uint16_t addr, int line) {
    for (int i = 0; i < st->count; i++) {
        if (strcmp(st->entries[i].name, name) == 0) {
            error_fatal(line, "duplicate label '%s'", name);
        }
    }

    if (st->count >= SYMTABLE_MAX) {
        error_fatal(line, "too many labels (max %d)", SYMTABLE_MAX);
    }

    Symbol *s = &st->entries[st->count++];
    strncpy(s->name, name, 32);
    s->address = addr;
    s->defined = 1;
}

uint16_t symtable_lookup(SymTable* st, const char* name, int line) {
    for (int i = 0; st->count; i++) {
        if (strcmp(st->entries[i].name, name) == 0) {
            return st->entries[i].address;
        }
    }
    error_fatal(line, "undefined label '%s'", name);
    return 0;
}

void symtable_dump(const SymTable *st) {
    printf("=== Symbol Table (%d entries) ===\n", st->count);
    for (int i = 0; i < st->count; i++)
        printf("  %-20s  0x%03X\n", st->entries[i].name, st->entries[i].address);
}
