#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdint.h>

#define SYMTABLE_MAX 256

typedef struct {
    char name[32];
    uint16_t address;
    int defined;
} Symbol;

typedef struct {
    Symbol entries[SYMTABLE_MAX];
    int count;
} SymTable;

/*
    === API ===
*/

/* Zero-initialise a symbol table. */
void symtable_init(SymTable* st);

/* Record label → address.  Calls error_fatal() on duplicate label. */
void symtable_define(SymTable* st, const char* name, uint16_t addr, int line);

/* Look up a label.  Returns the address, or calls error_fatal() if
 * the label was never defined. */
uint16_t symtable_lookup(SymTable* st, const char* name, int line);

/* Debug: print all entries. */
void      symtable_dump(const SymTable *st);

#endif